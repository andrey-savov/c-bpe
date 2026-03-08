"""
Benchmark c_bpe in the same format as bench_results.md (pytest-benchmark style).

Produces a markdown table with: tokens, min, mean, median, stddev, IQR, ops/s, ns/token, rounds
for each test case matching the rs-bpe benchmark suite.
"""
import sys, time, statistics, math

sys.path.insert(0, r"c:\Users\savov\source\forks\rs-bpe\c_bpe\python")
from c_bpe.openai import cl100k_base, o200k_base

# --- Text fixtures (same as test_benchmarks.py) ---
SMALL_TEXT = "This is a small test string for tokenization."
with open("README.md", "rb") as f:
    MEDIUM_TEXT_BYTES = f.read()
MEDIUM_TEXT_STR = MEDIUM_TEXT_BYTES.decode("utf-8")

# c_bpe takes bytes
SMALL_BYTES = SMALL_TEXT.encode("utf-8")


def bench(func, args=(), min_time_s=0.5, max_rounds=200000):
    """Run func(*args) repeatedly to collect timing stats like pytest-benchmark."""
    # Warmup
    for _ in range(10):
        func(*args)

    # Calibration: how many rounds to fill min_time_s
    t0 = time.perf_counter_ns()
    func(*args)
    t1 = time.perf_counter_ns()
    single_ns = max(t1 - t0, 50)  # at least 50ns
    target_rounds = max(20, int(min_time_s * 1e9 / single_ns))
    rounds = min(target_rounds, max_rounds)

    times_ns = []
    for _ in range(rounds):
        t0 = time.perf_counter_ns()
        result = func(*args)
        t1 = time.perf_counter_ns()
        times_ns.append(t1 - t0)

    times_ns.sort()
    n = len(times_ns)
    mn = min(times_ns)
    mean = statistics.mean(times_ns)
    med = statistics.median(times_ns)
    sd = statistics.stdev(times_ns) if n > 1 else 0
    q1 = times_ns[n // 4]
    q3 = times_ns[3 * n // 4]
    iqr = q3 - q1
    ops = 1e9 / mean if mean > 0 else 0

    return result, {
        "min": mn, "mean": mean, "median": med, "stddev": sd,
        "iqr": iqr, "ops": ops, "rounds": rounds
    }


def fmt_time(ns):
    """Format nanoseconds to human-readable."""
    if ns >= 1e6:
        return f"{ns/1e6:.3f} ms"
    elif ns >= 1e3:
        return f"{ns/1e3:.3f} μs"
    else:
        return f"{ns:.3f} ns"


def row(name, tokens, stats):
    s = stats
    ns_per_tok = s["median"] / tokens if tokens else 0
    return (
        f"| `{name}` | {tokens:,} | {fmt_time(s['min'])} | {fmt_time(s['mean'])} | "
        f"{fmt_time(s['median'])} | {fmt_time(s['stddev'])} | {fmt_time(s['iqr'])} | "
        f"{s['ops']:,.0f} | {ns_per_tok:.1f} ns | {s['rounds']} |"
    )


# Initialize tokenizers
cl100k = cl100k_base()
o200k_ = o200k_base()

# Pre-encode for decode benchmarks
cl100k_small_tokens = cl100k.encode(SMALL_BYTES)
cl100k_medium_tokens = cl100k.encode(MEDIUM_TEXT_BYTES)
o200k_small_tokens = o200k_.encode(SMALL_BYTES)

rows = []

# --- Encode ---
print("Running encode benchmarks...", flush=True)
r, s = bench(cl100k.encode, (SMALL_BYTES,))
rows.append(row("test_encode_cl100k[small]", len(r), s))

r, s = bench(cl100k.encode, (MEDIUM_TEXT_BYTES,))
rows.append(row("test_encode_cl100k[medium]", len(r), s))

# --- Decode ---
print("Running decode benchmarks...", flush=True)
r, s = bench(cl100k.decode, (cl100k_small_tokens,))
rows.append(row("test_decode_cl100k[small]", len(cl100k_small_tokens), s))

r, s = bench(o200k_.decode, (o200k_small_tokens,))
rows.append(row("test_decode_o200k[small]", len(o200k_small_tokens), s))

# --- Roundtrip ---
print("Running roundtrip benchmarks...", flush=True)
def roundtrip_cl100k(t):
    return cl100k.decode(cl100k.encode(t))
r, s = bench(roundtrip_cl100k, (SMALL_BYTES,))
rows.append(row("test_roundtrip_cl100k[small]", len(cl100k_small_tokens), s))

# --- Count ---
print("Running count benchmarks...", flush=True)
r, s = bench(cl100k.count, (SMALL_BYTES,))
rows.append(row("test_count_cl100k[small]", r, s))

# --- Count till limit ---
print("Running count_till_limit benchmarks...", flush=True)
r, s = bench(cl100k.count_till_limit, (SMALL_BYTES, 50))
tokens_counted = min(len(cl100k_small_tokens), 50)
rows.append(row("test_count_till_limit_cl100k[small]", tokens_counted, s))

# --- Batch ---
print("Running batch benchmarks...", flush=True)
batch_small = [SMALL_TEXT] * 100
def do_batch_small():
    return cl100k.encode_batch(batch_small)
r, s = bench(do_batch_small, (), min_time_s=0.5, max_rounds=10000)
tokens_list, total_tokens, _elapsed = r
rows.append(row("test_encode_batch_cl100k[small_x100]", total_tokens, s))

batch_medium = [MEDIUM_TEXT_STR] * 10
def do_batch_medium():
    return cl100k.encode_batch(batch_medium)
r, s = bench(do_batch_medium, (), min_time_s=0.5, max_rounds=500)
tokens_list, total_tokens, _elapsed = r
rows.append(row("test_encode_batch_cl100k[medium_x10]", total_tokens, s))

# --- o200k ---
print("Running o200k benchmarks...", flush=True)
r, s = bench(o200k_.encode, (SMALL_BYTES,))
rows.append(row("test_encode_o200k[small]", len(r), s))

r, s = bench(o200k_.encode, (MEDIUM_TEXT_BYTES,))
rows.append(row("test_encode_o200k[medium]", len(r), s))

def roundtrip_o200k(t):
    return o200k_.decode(o200k_.encode(t))
r, s = bench(roundtrip_o200k, (SMALL_BYTES,))
rows.append(row("test_roundtrip_o200k[small]", len(o200k_small_tokens), s))

# --- Output ---
print("\n\n## c_bpe Benchmark Results\n")
print("| Benchmark | tokens | min | mean | median | stddev | IQR | ops/s | ns/token | rounds |")
print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
for r in rows:
    print(r)
