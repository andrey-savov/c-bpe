"""Detailed profiling: regex vs BPE vs overhead breakdown."""
import sys, time, statistics
sys.path.insert(0, r"c:\Users\savov\source\forks\rs-bpe\c_bpe\python")

from c_bpe.openai import cl100k_base
tok = cl100k_base()

# Use the README as medium-sized text
with open("README.md", "rb") as f:
    text = f.read()
print(f"Text: {len(text)} bytes")

# Warm up
tok.encode(text)

# --- 1. Full encode ---
times = []
for _ in range(50):
    t0 = time.perf_counter_ns()
    tokens = tok.encode(text)
    t1 = time.perf_counter_ns()
    times.append((t1-t0)/1e6)
med = statistics.median(times)
print(f"\ntokenizer.encode: {med:.3f} ms ({len(tokens)} tokens, median of 50)")

# --- 2. Count only (no token list allocation) ---
times = []
for _ in range(50):
    t0 = time.perf_counter_ns()
    c = tok.count(text)
    t1 = time.perf_counter_ns()
    times.append((t1-t0)/1e6)
med_count = statistics.median(times)
print(f"tokenizer.count:  {med_count:.3f} ms (count={c}, median of 50)")
print(f"  => malloc/realloc overhead ≈ {med - med_count:.3f} ms")

# --- 3. Raw BPE (no regex) on pieces we know about ---
# Use the underlying BPE object to encode each pretok piece separately
bpe = tok.bpe()

# We can approximate pieces using Python regex (tiktoken's pattern)
import regex
pat = regex.compile(
    r"""(?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}"""
    r"""| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]+|\s+(?!\S)|\s+""")

pieces = pat.findall(text.decode("utf-8", errors="replace"))
print(f"\n{len(pieces)} pretok pieces (via Python regex)")

# Measure raw BPE time on those pieces
piece_bytes = [p.encode("utf-8") for p in pieces]

times_bpe = []
for _ in range(20):
    t0 = time.perf_counter_ns()
    total_toks = 0
    for pb in piece_bytes:
        c = bpe.count(pb)
        total_toks += c
    t1 = time.perf_counter_ns()
    times_bpe.append((t1-t0)/1e6)
med_bpe = statistics.median(times_bpe)
print(f"BPE-only (bpe.count on each piece): {med_bpe:.3f} ms ({total_toks} tokens)")

# Python overhead for the loop
times_loop = []
for _ in range(20):
    t0 = time.perf_counter_ns()
    for pb in piece_bytes:
        pass  # just loop overhead
    t1 = time.perf_counter_ns()
    times_loop.append((t1-t0)/1e6)
med_loop = statistics.median(times_loop)
print(f"Python loop overhead:               {med_loop:.3f} ms")
print(f"BPE-only (minus loop):              {med_bpe - med_loop:.3f} ms")

# --- 4. Regex-only time estimate ---
print(f"\nEstimated breakdown:")
regex_time = med_count - (med_bpe - med_loop)
print(f"  Regex (PCRE2):    {regex_time:.3f} ms")
print(f"  BPE (backtrack):  {med_bpe - med_loop:.3f} ms")
print(f"  Token alloc:      {med - med_count:.3f} ms")
print(f"  Total:            {med:.3f} ms")

# --- 5. tiktoken comparison ---
try:
    import tiktoken
    enc = tiktoken.get_encoding("cl100k_base")
    times_tk = []
    for _ in range(50):
        t0 = time.perf_counter_ns()
        toks_tk = enc.encode(text.decode("utf-8"))
        t1 = time.perf_counter_ns()
        times_tk.append((t1-t0)/1e6)
    med_tk = statistics.median(times_tk)
    print(f"\ntiktoken:           {med_tk:.3f} ms ({len(toks_tk)} tokens)")
except ImportError:
    print("\ntiktoken not available")

# --- 6. Per-piece BPE distribution ---
print(f"\nPer-piece BPE analysis:")
piece_lens = [len(pb) for pb in piece_bytes]
print(f"  Pieces: {len(piece_lens)}")
print(f"  Avg piece len: {sum(piece_lens)/len(piece_lens):.1f} bytes")
print(f"  Max piece len: {max(piece_lens)} bytes")
print(f"  Pieces > 10B: {sum(1 for l in piece_lens if l > 10)}")
print(f"  Pieces <= 3B: {sum(1 for l in piece_lens if l <= 3)}")

# Measure bpe.count call overhead for tiny pieces
tiny = b"a"
times_tiny = []
for _ in range(10000):
    t0 = time.perf_counter_ns()
    bpe.count(tiny)
    t1 = time.perf_counter_ns()
    times_tiny.append((t1-t0)/1e6)
med_tiny = statistics.median(times_tiny)
print(f"\n  Single bpe.count(b'a'): {med_tiny*1000:.1f} ns")
print(f"  × {len(piece_lens)} pieces = {med_tiny * len(piece_lens):.3f} ms (call overhead alone)")
