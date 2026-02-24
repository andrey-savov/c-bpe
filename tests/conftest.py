"""
Test configuration and fixtures for rs_bpe module tests.

Benchmark markdown export
-------------------------
After any run that includes benchmark tests, a Markdown summary table is
written to ``benchmark/bench_results.md`` automatically.  No extra flags are
needed – just run::

    pytest tests/test_benchmarks.py --benchmark-only --benchmark-sort=mean
"""

from __future__ import annotations

from pathlib import Path


# ---------------------------------------------------------------------------
# Benchmark markdown reporter
# ---------------------------------------------------------------------------

def pytest_sessionfinish(session, exitstatus):  # noqa: ANN001
    """Write a Markdown benchmark table after the session when benchmarks ran."""
    plugin = session.config.pluginmanager.get_plugin("pytest-benchmark")
    if plugin is None:
        return

    # The registered object IS a BenchmarkSession; .benchmarks is the list
    benchmarks = getattr(plugin, "benchmarks", None)
    if not benchmarks:
        return

    rows = []
    for bench in benchmarks:
        if not bench:                        # skipped / disabled
            continue
        stats = bench.stats
        rows.append(
            {
                "name":    bench.name,
                "min":     stats.min,
                "mean":    stats.mean,
                "median":  stats.median,
                "stddev":  stats.stddev,
                "iqr":     stats.iqr,
                "ops":     stats.ops,
                "rounds":  stats.rounds,
            }
        )

    if not rows:
        return

    def _fmt_time(seconds: float) -> str:
        """Auto-scale seconds to a human-readable unit."""
        if seconds >= 1:
            return f"{seconds:.3f} s"
        if seconds >= 1e-3:
            return f"{seconds * 1e3:.3f} ms"
        if seconds >= 1e-6:
            return f"{seconds * 1e6:.3f} μs"
        return f"{seconds * 1e9:.3f} ns"

    header = "| Benchmark | min | mean | median | stddev | IQR | ops/s | rounds |"
    sep    = "| --- | --- | --- | --- | --- | --- | --- | --- |"
    lines  = [header, sep]
    for r in rows:
        lines.append(
            f"| `{r['name']}` "
            f"| {_fmt_time(r['min'])} "
            f"| {_fmt_time(r['mean'])} "
            f"| {_fmt_time(r['median'])} "
            f"| {_fmt_time(r['stddev'])} "
            f"| {_fmt_time(r['iqr'])} "
            f"| {r['ops']:,.0f} "
            f"| {r['rounds']} |"
        )

    out = Path("benchmark/bench_results.md")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("# Benchmark Results\n\n" + "\n".join(lines) + "\n",
                   encoding="utf-8")
    session.config.pluginmanager.get_plugin("terminalreporter").write_line(
        f"\nbenchmark markdown → {out}"
    )
