"""Profile scaling: how does per-piece cost change with text complexity?"""
import sys, time, statistics
sys.path.insert(0, r"c:\Users\savov\source\forks\rs-bpe\c_bpe\python")
from c_bpe.openai import cl100k_base
import tiktoken

tok = cl100k_base()
enc = tiktoken.get_encoding("cl100k_base")

with open("README.md", "rb") as f:
    readme = f.read()

# Test at different text sizes
for size in [100, 500, 1000, 2000, 5000, 10000, 15000]:
    text = readme[:size]
    text_str = text.decode("utf-8", errors="replace")
    
    # warmup
    tok.count(text)
    enc.encode(text_str)
    
    n_runs = max(10, 200000 // (size + 1))
    
    times_c = []
    for _ in range(n_runs):
        t0 = time.perf_counter_ns()
        c = tok.count(text)
        t1 = time.perf_counter_ns()
        times_c.append((t1 - t0) / 1e6)
    
    times_tk = []
    for _ in range(n_runs):
        t0 = time.perf_counter_ns()
        enc.encode(text_str)
        t1 = time.perf_counter_ns()
        times_tk.append((t1 - t0) / 1e6)
    
    mc = statistics.median(times_c)
    mtk = statistics.median(times_tk)
    ratio = mc / mtk if mtk > 0 else float('inf')
    print(f"{size:6d}B: c_bpe={mc:.3f}ms  tiktoken={mtk:.3f}ms  ratio={ratio:.1f}×  tokens={c}")
