"""
Benchmark script comparing c_bpe, rs_bpe, and tiktoken on cl100k_base.

Measures encoding, decoding, and roundtrip performance across small/medium/large
text sizes and produces SVG charts saved to the benchmark/ directory.

Usage:
    python benchmark/benchmark.py
"""

import gc
import statistics
import sys
import time
from abc import ABC, abstractmethod
from typing import Any, cast

import matplotlib.pyplot as plt
import numpy as np
import tiktoken
from matplotlib.ticker import FuncFormatter

# ---------------------------------------------------------------------------
# Optional imports
# ---------------------------------------------------------------------------

try:
    import rs_bpe
    RS_BPE_AVAILABLE = True
except ImportError:
    print("Warning: rs_bpe module not available (run 'maturin develop' to build).")
    RS_BPE_AVAILABLE = False

try:
    import c_bpe
    C_BPE_AVAILABLE = True
except ImportError:
    print("Warning: c_bpe module not available (run 'pip install -e c_bpe/' to build).")
    C_BPE_AVAILABLE = False

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
    "small":  SMALL_TEXT,
    "medium": MEDIUM_TEXT,
    "large":  LARGE_TEXT,
}


# ---------------------------------------------------------------------------
# Adapters
# ---------------------------------------------------------------------------

class TokenizerAdapter(ABC):
    def __init__(self, name: str):
        self.name = name

    @abstractmethod
    def encode(self, text: str) -> "list[int]": ...

    @abstractmethod
    def decode(self, tokens: "list[int]") -> str: ...

    def encode_batch(self, texts: "list[str]") -> "list[list[int]]":
        return [self.encode(t) for t in texts]


class TiktokenAdapter(TokenizerAdapter):
    def __init__(self):
        super().__init__("tiktoken")
        self.tokenizer = tiktoken.get_encoding("cl100k_base")

    def encode(self, text: str) -> "list[int]":
        return self.tokenizer.encode(text)

    def decode(self, tokens: "list[int]") -> str:
        return self.tokenizer.decode(tokens)

    def encode_batch(self, texts: "list[str]") -> "list[list[int]]":
        return self.tokenizer.encode_batch(texts)


class RsBpeAdapter(TokenizerAdapter):
    def __init__(self):
        super().__init__("rs_bpe")
        if not RS_BPE_AVAILABLE:
            raise ImportError("rs_bpe not available")
        self.tokenizer = rs_bpe.bpe.openai.cl100k_base()

    def encode(self, text: str) -> "list[int]":
        return cast("list[int]", self.tokenizer.encode(text))

    def decode(self, tokens: "list[int]") -> str:
        result = self.tokenizer.decode(tokens)
        return result if result is not None else ""

    def encode_batch(self, texts: "list[str]") -> "list[list[int]]":
        result, _total, _elapsed = self.tokenizer.encode_batch(texts)
        return result


class CBpeAdapter(TokenizerAdapter):
    def __init__(self):
        super().__init__("c_bpe")
        if not C_BPE_AVAILABLE:
            raise ImportError("c_bpe not available")
        self.tokenizer = c_bpe.bpe.openai.cl100k_base()

    def encode(self, text: str) -> "list[int]":
        return cast("list[int]", self.tokenizer.encode(text))

    def decode(self, tokens: "list[int]") -> str:
        result = self.tokenizer.decode(tokens)
        return result if result is not None else ""

    def encode_batch(self, texts: "list[str]") -> "list[list[int]]":
        result, _total, _elapsed = self.tokenizer.encode_batch(texts)
        return result


# ---------------------------------------------------------------------------
# Benchmark runner
# ---------------------------------------------------------------------------

class BenchmarkRunner:
    def __init__(self, tokenizers: "list[TokenizerAdapter]", num_runs: int = 5):
        self.tokenizers = tokenizers
        self.num_runs = num_runs
        self.results: dict = {
            "encode": {},
            "decode": {},
            "roundtrip": {},
            "token_count": {},
        }

    def _time_median(self, fn, *args) -> "tuple[float, Any]":
        times = []
        result = None
        for _ in range(self.num_runs):
            gc.collect()
            t0 = time.perf_counter()
            result = fn(*args)
            times.append(time.perf_counter() - t0)
        return statistics.median(times), result

    def benchmark_encode(self, size: str) -> None:
        text = TEST_TEXTS[size]
        self.results["encode"].setdefault(size, {})
        self.results["token_count"].setdefault(size, {})
        for tok in self.tokenizers:
            dt, tokens = self._time_median(tok.encode, text)
            self.results["encode"][size][tok.name] = dt
            self.results["token_count"][size][tok.name] = len(tokens) if tokens else 0

    def benchmark_decode(self, size: str) -> None:
        text = TEST_TEXTS[size]
        self.results["decode"].setdefault(size, {})
        for tok in self.tokenizers:
            tokens = tok.encode(text)
            dt, _ = self._time_median(tok.decode, tokens)
            self.results["decode"][size][tok.name] = dt

    def benchmark_roundtrip(self, size: str) -> None:
        text = TEST_TEXTS[size]
        self.results["roundtrip"].setdefault(size, {})
        for tok in self.tokenizers:
            def rt(t=text, adapter=tok):
                return adapter.decode(adapter.encode(t))
            dt, _ = self._time_median(rt)
            self.results["roundtrip"][size][tok.name] = dt

    def run_benchmarks(self) -> dict:
        for size in TEST_TEXTS:
            print(f"  {size}...", flush=True)
            self.benchmark_encode(size)
            self.benchmark_decode(size)
            self.benchmark_roundtrip(size)
        return self.results

    def print_results(self) -> None:
        names = [t.name for t in self.tokenizers]
        sizes = list(TEST_TEXTS.keys())
        ops   = ["encode", "decode", "roundtrip"]

        for op in ops:
            print(f"\n{op.upper()} (median ms):")
            print(f"  {'size':<8}", end="")
            for n in names:
                print(f"  {n:>12}", end="")
            print()
            for size in sizes:
                print(f"  {size:<8}", end="")
                for n in names:
                    t = self.results[op][size][n]
                    print(f"  {t*1000:>11.3f}ms", end="")
                print()

        # Speedup table vs tiktoken
        if "tiktoken" in names and len(names) > 1:
            print("\nSPEEDUP vs tiktoken (encode, median):")
            print(f"  {'size':<8}", end="")
            for n in names:
                if n != "tiktoken":
                    print(f"  {n:>12}", end="")
            print()
            for size in sizes:
                print(f"  {size:<8}", end="")
                base = self.results["encode"][size]["tiktoken"]
                for n in names:
                    if n != "tiktoken":
                        t = self.results["encode"][size][n]
                        ratio = base / t if t > 0 else 0
                        print(f"  {ratio:>11.2f}×", end="")
                print()

    def plot_results(self, save_dir: str = "benchmark") -> None:
        names   = [t.name for t in self.tokenizers]
        sizes   = list(TEST_TEXTS.keys())
        ops     = ["encode", "decode", "roundtrip"]
        colors  = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"]
        markers = ["o", "s", "d", "^", "v"]

        byte_sizes = [len(TEST_TEXTS[s].encode("utf-8")) for s in sizes]
        xlabels = [f"{s}\n({_fmt_bytes(b)})" for s, b in zip(sizes, byte_sizes)]
        x = np.arange(len(sizes))

        # --- Time comparison (log scale) ---
        fig, axs = plt.subplots(len(ops), 1, figsize=(10, 4 * len(ops)))
        fig.suptitle("Tokenizer Performance: c_bpe vs rs_bpe vs tiktoken", fontsize=14, fontweight="bold")
        for i, op in enumerate(ops):
            ax = axs[i]
            for j, name in enumerate(names):
                times = [self.results[op][s][name] for s in sizes]
                ax.plot(x, times, marker=markers[j % len(markers)], linewidth=2,
                        markersize=7, label=name, color=colors[j % len(colors)])
            ax.set_yscale("log")
            ax.set_title(f"{op.capitalize()} time (lower is better)", fontsize=12)
            ax.set_ylabel("seconds (log scale)")
            ax.set_xticks(x); ax.set_xticklabels(xlabels)
            ax.grid(True, linestyle="--", alpha=0.6)
            ax.legend()
        fig.tight_layout(rect=(0, 0, 1, 0.96))
        fig.savefig(f"{save_dir}/tokenizer_benchmark_results_time.svg", format="svg", bbox_inches="tight")
        print(f"  → {save_dir}/tokenizer_benchmark_results_time.svg")

        # --- Encoding throughput ---
        fig2, ax2 = plt.subplots(figsize=(10, 5))
        fig2.suptitle("Encoding Throughput (higher is better)", fontsize=14, fontweight="bold")
        for j, name in enumerate(names):
            tps = [
                self.results["token_count"][s][name] / self.results["encode"][s][name]
                for s in sizes
            ]
            ax2.plot(x, tps, marker=markers[j % len(markers)], linewidth=2,
                     markersize=7, label=name, color=colors[j % len(colors)])
        ax2.set_ylabel("tokens / second")
        ax2.set_xticks(x); ax2.set_xticklabels(xlabels)
        ax2.grid(True, linestyle="--", alpha=0.6)
        ax2.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{int(v):,}"))
        ax2.legend()
        fig2.tight_layout(rect=(0, 0, 1, 0.96))
        fig2.savefig(f"{save_dir}/tokenizer_benchmark_results_throughput.svg", format="svg", bbox_inches="tight")
        print(f"  → {save_dir}/tokenizer_benchmark_results_throughput.svg")

        # --- Speedup vs tiktoken ---
        if "tiktoken" in names:
            others = [n for n in names if n != "tiktoken"]
            fig3, ax3 = plt.subplots(figsize=(10, 5))
            fig3.suptitle("Encode speedup vs tiktoken (higher is better)", fontsize=14, fontweight="bold")
            for name in others:
                speedups = [
                    self.results["encode"][s]["tiktoken"] / self.results["encode"][s][name]
                    for s in sizes
                ]
                ci = names.index(name)
                ax3.plot(x, speedups, marker=markers[ci % len(markers)], linewidth=2,
                         markersize=7, label=name, color=colors[ci % len(colors)])
            ax3.axhline(1.0, color="gray", linestyle="--", alpha=0.6, label="tiktoken baseline")
            ax3.set_ylabel("speedup ratio")
            ax3.set_xticks(x); ax3.set_xticklabels(xlabels)
            ax3.grid(True, linestyle="--", alpha=0.6)
            ax3.legend()
            fig3.tight_layout(rect=(0, 0, 1, 0.96))
            fig3.savefig(f"{save_dir}/tokenizer_benchmark_results_speedup.svg", format="svg", bbox_inches="tight")
            print(f"  → {save_dir}/tokenizer_benchmark_results_speedup.svg")

        plt.show()


def _fmt_bytes(n: int) -> str:
    if n >= 1_000_000:
        return f"{n / 1_000_000:.1f} MB"
    if n >= 1_000:
        return f"{n / 1_000:.1f} KB"
    return f"{n} B"


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def run_benchmarks() -> None:
    print("Initialising tokenizers...")
    adapters: list[TokenizerAdapter] = []

    try:
        adapters.append(TiktokenAdapter());  print("  [OK] tiktoken")
    except Exception as e:
        print(f"  [SKIP] tiktoken: {e}")

    if RS_BPE_AVAILABLE:
        try:
            adapters.append(RsBpeAdapter());  print("  [OK] rs_bpe")
        except Exception as e:
            print(f"  [SKIP] rs_bpe: {e}")

    if C_BPE_AVAILABLE:
        try:
            adapters.append(CBpeAdapter());  print("  [OK] c_bpe")
        except Exception as e:
            print(f"  [SKIP] c_bpe: {e}")

    if not adapters:
        print("No tokenizers available.")
        sys.exit(1)

    print("\nRunning benchmarks (7 runs each)...")
    runner = BenchmarkRunner(adapters, num_runs=7)
    runner.run_benchmarks()

    runner.print_results()

    print("\nGenerating charts...")
    runner.plot_results(save_dir="benchmark")
    print("Done.")


if __name__ == "__main__":
    run_benchmarks()
