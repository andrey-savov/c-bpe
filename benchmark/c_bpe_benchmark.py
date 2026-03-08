"""
Benchmark script for comparing c_bpe (C extension) against tiktoken.

Measures:
  - Construction time (tokenizer init)
  - Single-string encode throughput (small / medium / large)
  - Single-string decode throughput
  - Encode-decode roundtrip
  - Batch encode (sequential and parallel)

Usage:
    python benchmark/c_bpe_benchmark.py
"""

import gc
import statistics
import sys
import time

# ---------------------------------------------------------------------------
# Test data
# ---------------------------------------------------------------------------
SMALL_TEXT = "This is a small test string for tokenization."
try:
    with open("README.md", "r") as f:
        MEDIUM_TEXT = f.read()
except FileNotFoundError:
    MEDIUM_TEXT = SMALL_TEXT * 20

LARGE_TEXT = MEDIUM_TEXT * 50

TEST_TEXTS = {
    "small": SMALL_TEXT,
    "medium": MEDIUM_TEXT,
    "large": LARGE_TEXT,
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _fmt_bytes(n: int) -> str:
    if n >= 1_000_000:
        return f"{n / 1_000_000:.1f} MB"
    if n >= 1_000:
        return f"{n / 1_000:.1f} KB"
    return f"{n} B"


def _time(fn, *args, warmup: int = 1, runs: int = 7):
    """Return (median_seconds, last_result)."""
    for _ in range(warmup):
        fn(*args)
    times = []
    result = None
    for _ in range(runs):
        gc.collect()
        t0 = time.perf_counter()
        result = fn(*args)
        times.append(time.perf_counter() - t0)
    return statistics.median(times), result


# ---------------------------------------------------------------------------
# Adapters
# ---------------------------------------------------------------------------

class CBpeAdapter:
    name = "c_bpe"

    def __init__(self):
        from c_bpe.bpe.openai import cl100k_base
        self._factory = cl100k_base
        self.tok = cl100k_base()

    def encode(self, text: str) -> list:
        return self.tok.encode(text.encode("utf-8") if isinstance(text, str) else text)

    def decode(self, tokens: list) -> str:
        return self.tok.decode(tokens)

    def encode_batch(self, texts: list[str]) -> list:
        r, _total, _elapsed = self.tok.encode_batch(texts)
        return r

    def construction_time(self, runs: int = 5) -> float:
        """Time a fresh tokenizer construction (forces rebuild)."""
        times = []
        for _ in range(runs):
            gc.collect()
            # Reimport to force reconstruction
            import importlib, c_bpe.bpe.openai as mod
            # The module caches singletons; we can't easily force a rebuild
            # from Python.  Instead, measure cl100k_base() which returns the
            # cached singleton after the first call.  Only the first call pays
            # the real cost.  We report that first-call time separately.
            # For a fair construction benchmark we just time the first import
            # in a subprocess.
            break
        # Fall back to subprocess measurement
        import subprocess
        cmd = [
            sys.executable, "-c",
            "import time; t0=time.perf_counter(); "
            "from c_bpe.bpe.openai import cl100k_base; t=cl100k_base(); "
            "print(f'{time.perf_counter()-t0:.6f}')"
        ]
        for _ in range(runs):
            out = subprocess.check_output(cmd, text=True).strip()
            times.append(float(out))
        return statistics.median(times)


class TiktokenAdapter:
    name = "tiktoken"

    def __init__(self):
        import tiktoken
        self._tiktoken = tiktoken
        self.tok = tiktoken.get_encoding("cl100k_base")

    def encode(self, text: str) -> list:
        return self.tok.encode(text)

    def decode(self, tokens: list) -> str:
        return self.tok.decode(tokens)

    def encode_batch(self, texts: list[str]) -> list:
        return self.tok.encode_batch(texts)

    def construction_time(self, runs: int = 5) -> float:
        import subprocess
        cmd = [
            sys.executable, "-c",
            "import time; t0=time.perf_counter(); "
            "import tiktoken; t=tiktoken.get_encoding('cl100k_base'); "
            "print(f'{time.perf_counter()-t0:.6f}')"
        ]
        times = []
        for _ in range(runs):
            out = subprocess.check_output(cmd, text=True).strip()
            times.append(float(out))
        return statistics.median(times)


# ---------------------------------------------------------------------------
# Benchmark runner
# ---------------------------------------------------------------------------

def run_benchmarks():
    adapters: list = []

    # c_bpe
    try:
        adapters.append(CBpeAdapter())
        print("[OK] c_bpe loaded")
    except Exception as e:
        print(f"[SKIP] c_bpe: {e}")

    # tiktoken
    try:
        adapters.append(TiktokenAdapter())
        print("[OK] tiktoken loaded")
    except Exception as e:
        print(f"[SKIP] tiktoken: {e}")

    if not adapters:
        print("No tokenizers available!")
        return

    # ------------------------------------------------------------------
    # 1. Construction time
    # ------------------------------------------------------------------
    print("\n===== CONSTRUCTION TIME (cold start, median of 5) =====")
    for a in adapters:
        t = a.construction_time(runs=5)
        print(f"  {a.name:12s}: {t:.4f}s")

    # ------------------------------------------------------------------
    # 2. Encoding
    # ------------------------------------------------------------------
    print("\n===== ENCODING (median of 7 runs) =====")
    for size_name, text in TEST_TEXTS.items():
        nbytes = len(text.encode("utf-8"))
        print(f"\n  {size_name} ({_fmt_bytes(nbytes)}):")
        for a in adapters:
            dt, tokens = _time(a.encode, text)
            ntok = len(tokens)
            tps = ntok / dt if dt > 0 else float("inf")
            print(f"    {a.name:12s}: {dt*1000:8.3f} ms  "
                  f"({ntok:,} tokens, {tps:,.0f} tok/s)")

    # ------------------------------------------------------------------
    # 3. Decoding
    # ------------------------------------------------------------------
    print("\n===== DECODING (median of 7 runs) =====")
    for size_name, text in TEST_TEXTS.items():
        nbytes = len(text.encode("utf-8"))
        tokens = adapters[0].encode(text)
        print(f"\n  {size_name} ({_fmt_bytes(nbytes)}, {len(tokens):,} tokens):")
        for a in adapters:
            # Each adapter must decode its own tokens
            own_tokens = a.encode(text)
            dt, _ = _time(a.decode, own_tokens)
            print(f"    {a.name:12s}: {dt*1000:8.3f} ms")

    # ------------------------------------------------------------------
    # 4. Roundtrip
    # ------------------------------------------------------------------
    print("\n===== ROUNDTRIP encode+decode (median of 7 runs) =====")
    for size_name, text in TEST_TEXTS.items():
        nbytes = len(text.encode("utf-8"))
        print(f"\n  {size_name} ({_fmt_bytes(nbytes)}):")
        for a in adapters:
            def roundtrip(t=text, adapter=a):
                return adapter.decode(adapter.encode(t))
            dt, _ = _time(roundtrip)
            print(f"    {a.name:12s}: {dt*1000:8.3f} ms")

    # ------------------------------------------------------------------
    # 5. Batch encoding
    # ------------------------------------------------------------------
    print("\n===== BATCH ENCODE (1000 texts, median of 5 runs) =====")
    import random
    random.seed(42)
    words = MEDIUM_TEXT.split()
    batch = []
    for _ in range(1000):
        length = random.randint(20, 200)
        batch.append(" ".join(random.choices(words, k=length)))

    total_bytes = sum(len(t.encode("utf-8")) for t in batch)
    print(f"  {len(batch)} texts, {_fmt_bytes(total_bytes)} total")
    for a in adapters:
        dt, result = _time(a.encode_batch, batch, warmup=1, runs=5)
        total_tokens = sum(len(r) for r in result)
        print(f"    {a.name:12s}: {dt*1000:8.1f} ms  "
              f"({total_tokens:,} tokens, {total_tokens/dt:,.0f} tok/s)")

    # ------------------------------------------------------------------
    # 6. Correctness cross-check
    # ------------------------------------------------------------------
    if len(adapters) >= 2:
        print("\n===== CORRECTNESS CROSS-CHECK =====")
        test_strings = [
            "Hello, world!",
            "The quick brown fox jumps over the lazy dog",
            "   leading spaces",
            "def fibonacci(n):\n    if n <= 1:\n        return n\n    return fibonacci(n-1) + fibonacci(n-2)",
            "GPT-4 is great!",
            "numbers 12345 67890",
            "special !@#$%^&*()",
            "\t\ttabs\nand\nnewlines",
        ]
        a0, a1 = adapters[0], adapters[1]
        ok = 0
        for s in test_strings:
            t0 = a0.encode(s)
            t1 = a1.encode(s)
            if t0 == t1:
                ok += 1
            else:
                print(f"  MISMATCH: {s[:40]!r}")
                print(f"    {a0.name}: {t0[:10]}...")
                print(f"    {a1.name}: {t1[:10]}...")
        print(f"  {ok}/{len(test_strings)} matched between {a0.name} and {a1.name}")


if __name__ == "__main__":
    run_benchmarks()
