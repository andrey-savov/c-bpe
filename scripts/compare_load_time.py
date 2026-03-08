"""Compare import + cl100k_base() load time for c_bpe vs rs_bpe."""
import importlib
import statistics
import sys
import time

WARMUP = 1
RUNS = 10

results = {}
for name in ["c_bpe.bpe", "rs_bpe.bpe"]:
    times = []
    for i in range(WARMUP + RUNS):
        # Force re-import
        for k in list(sys.modules):
            if k.startswith(name.split(".")[0]):
                del sys.modules[k]
        t0 = time.perf_counter()
        mod = importlib.import_module(name)
        tok = mod.openai.cl100k_base()
        t1 = time.perf_counter()
        if i >= WARMUP:
            times.append((t1 - t0) * 1000)
    results[name] = times
    med = statistics.median(times)
    avg = statistics.mean(times)
    mn = min(times)
    runs_str = ", ".join(f"{t:.1f}" for t in times)
    print(f"{name}:  median={med:.1f}ms  mean={avg:.1f}ms  min={mn:.1f}ms  [{runs_str}]")

m_c = statistics.median(results["c_bpe.bpe"])
m_r = statistics.median(results["rs_bpe.bpe"])
faster = "c_bpe" if m_c < m_r else "rs_bpe"
ratio = max(m_c, m_r) / min(m_c, m_r)
print(f"\n{faster} is {ratio:.2f}x faster to load (median)")
