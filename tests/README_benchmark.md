# Benchmark Tests

Benchmarks compare **rs_bpe** (Rust) and **c_bpe** (C) tokenizer implementations
side-by-side using [pytest-benchmark](https://pytest-benchmark.readthedocs.io/).

## Running

```bash
# All benchmarks
pytest tests/test_benchmarks.py --benchmark-only

# Sort by mean time
pytest tests/test_benchmarks.py --benchmark-only --benchmark-sort=mean

# One implementation only
pytest tests/test_benchmarks.py --benchmark-only -k rs_bpe
pytest tests/test_benchmarks.py --benchmark-only -k c_bpe

# Skip benchmarks, run correctness checks only
pytest tests/test_benchmarks.py --benchmark-disable
```

## Results

A Markdown report is auto-generated after every benchmark run (handled by the
`conftest.py` session-finish hook) into a platform-specific file:

- **`tests/bench_results_windows.md`** — Windows
- **`tests/bench_results_linux.md`** — Linux / WSL
- **`tests/bench_results_darwin.md`** — macOS

The report contains per-implementation tables and a head-to-head comparison of
median times.

## Prerequisites

```bash
pip install pytest pytest-benchmark
pip install -e .          # rs_bpe (Rust, via maturin)
pip install -e c_bpe/     # c_bpe  (C, via setuptools)
```
