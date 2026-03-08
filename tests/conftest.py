"""
Test configuration and fixtures for the combined rs_bpe / c_bpe benchmark suite.

Benchmark markdown export
-------------------------
After any run that includes benchmark tests, a Markdown report is written to
``benchmark/bench_results.md`` automatically.  No extra flags are needed::

    pytest tests/test_benchmarks.py --benchmark-only

The report contains:
  - A table for each implementation (rs_bpe, c_bpe, and any others found)
  - A comparison table (median times and ratio) for matching test names
"""

from __future__ import annotations

import re
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _fmt_time(seconds: float) -> str:
    """Auto-scale seconds to a human-readable unit."""
    if seconds >= 1:
        return f"{seconds:.3f} s"
    if seconds >= 1e-3:
        return f"{seconds * 1e3:.3f} ms"
    if seconds >= 1e-6:
        return f"{seconds * 1e6:.3f} μs"
    return f"{seconds * 1e9:.3f} ns"


def _ns_per_token(mean_s: float, tokens: int | None) -> str:
    if not tokens:
        return "n/a"
    return f"{mean_s / tokens * 1e9:.1f} ns"


def _split_impl(bench_name: str) -> tuple[str, str]:
    """Return (impl, base_name) by parsing parametrize ids.

    'test_encode_cl100k[rs_bpe-small]' → ('rs_bpe', 'test_encode_cl100k[small]')
    'test_encode_cl100k[small]'        → ('unknown', 'test_encode_cl100k[small]')
    """
    m = re.match(r'^(.*)\[(rs_bpe|c_bpe)-(.*)\]$', bench_name)
    if m:
        return m.group(2), f"{m.group(1)}[{m.group(3)}]"
    return "unknown", bench_name


_TABLE_HEADER = "| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |"
_TABLE_SEP    = "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"


def _bench_row(r: dict) -> str:
    tok_str = f"{r['tokens']:,}" if r["tokens"] else "n/a"
    return (
        f"| `{r['name']}` "
        f"| {tok_str} "
        f"| {_fmt_time(r['min'])} "
        f"| {_fmt_time(r['mean'])} "
        f"| {_fmt_time(r['median'])} "
        f"| {_fmt_time(r['stddev'])} "
        f"| {_fmt_time(r['iqr'])} "
        f"| {r['ops']:,.0f} "
        f"| {_ns_per_token(r['mean'], r['tokens'])} "
        f"| {r['rounds']} |"
    )


def _impl_table(impl: str, rows: list[dict]) -> list[str]:
    lines = [f"## {impl}", "", _TABLE_HEADER, _TABLE_SEP]
    for r in rows:
        lines.append(_bench_row(r))
    return lines


def _comparison_table(
    impl_a: str, rows_a: dict[str, dict],
    impl_b: str, rows_b: dict[str, dict],
) -> list[str]:
    """Produce a median-time comparison table for matching benchmark names."""
    common = sorted(k for k in rows_a if k in rows_b)
    if not common:
        return []

    lines = [
        f"## Comparison (median, {impl_a} vs {impl_b})",
        "",
        f"| Benchmark | {impl_a} | {impl_b} | ratio |",
        "| --- | --- | --- | --- |",
    ]
    for base in common:
        ra = rows_a[base]
        rb = rows_b[base]
        ma = ra["median"]
        mb = rb["median"]
        ratio = ma / mb if mb > 0 else float("inf")
        # Bold when impl_b is faster (ratio < 1)
        ratio_str = f"**{ratio:.2f}×**" if ratio < 1 else f"{ratio:.2f}×"
        lines.append(
            f"| `{base}` "
            f"| {_fmt_time(ma)} "
            f"| {_fmt_time(mb)} "
            f"| {ratio_str} |"
        )
    return lines


# ---------------------------------------------------------------------------
# pytest hook
# ---------------------------------------------------------------------------

def pytest_sessionfinish(session, exitstatus):  # noqa: ANN001
    """Write a Markdown benchmark report after the session when benchmarks ran."""
    plugin = session.config.pluginmanager.get_plugin("pytest-benchmark")
    if plugin is None:
        return

    benchmarks = getattr(plugin, "benchmarks", None)
    if not benchmarks:
        return

    # Collect all rows, grouped by implementation
    by_impl: dict[str, list[dict]] = {}
    for bench in benchmarks:
        if not bench:
            continue
        stats = bench.stats
        impl, base_name = _split_impl(bench.name)
        row = {
            "name":      base_name,
            "full_name": bench.name,
            "tokens":    bench.extra_info.get("tokens"),
            "min":       stats.min,
            "mean":      stats.mean,
            "median":    stats.median,
            "stddev":    stats.stddev,
            "iqr":       stats.iqr,
            "ops":       stats.ops,
            "rounds":    stats.rounds,
        }
        by_impl.setdefault(impl, []).append(row)

    if not by_impl:
        return

    sections: list[str] = ["# Benchmark Results", ""]

    # Per-implementation tables (rs_bpe first, then c_bpe, then others)
    order = ["rs_bpe", "c_bpe"] + sorted(k for k in by_impl if k not in ("rs_bpe", "c_bpe"))
    for impl in order:
        if impl not in by_impl:
            continue
        sections += _impl_table(impl, by_impl[impl])
        sections.append("")

    # Comparison table between rs_bpe and c_bpe if both present
    if "rs_bpe" in by_impl and "c_bpe" in by_impl:
        rs_by_base = {r["name"]: r for r in by_impl["rs_bpe"]}
        c_by_base  = {r["name"]: r for r in by_impl["c_bpe"]}
        sections += _comparison_table("rs_bpe", rs_by_base, "c_bpe", c_by_base)
        sections.append("")

    out = Path("benchmark/bench_results.md")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(sections), encoding="utf-8")
    session.config.pluginmanager.get_plugin("terminalreporter").write_line(
        f"\nbenchmark markdown → {out}"
    )

