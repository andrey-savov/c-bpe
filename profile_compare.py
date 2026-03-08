"""Head-to-head: c_bpe vs rs-bpe vs tiktoken on cl100k_base."""
import sys, time, statistics

# --- Load all three tokenizers ---
sys.path.insert(0, r"c:\Users\savov\source\forks\rs-bpe\c_bpe\python")

from c_bpe.openai import cl100k_base as c_cl100k
from rs_bpe.bpe import openai as rs_openai
import tiktoken

c_tok  = c_cl100k()
rs_tok = rs_openai.cl100k_base()
tk_enc = tiktoken.get_encoding("cl100k_base")

with open("README.md", "rb") as f:
    text_bytes = f.read()
text_str = text_bytes.decode("utf-8")

print(f"Text: {len(text_bytes)} bytes\n")

# Verify identical output
c_tokens  = c_tok.encode(text_bytes)
rs_tokens = rs_tok.encode(text_str)
tk_tokens = tk_enc.encode(text_str)
assert list(c_tokens) == list(rs_tokens) == list(tk_tokens), \
    f"Token mismatch! c={len(c_tokens)} rs={len(rs_tokens)} tk={len(tk_tokens)}"
print(f"All three produce {len(c_tokens)} tokens (verified identical)\n")

# --- Benchmark at different sizes ---
print(f"{'Size':>7s}  {'c_bpe':>8s}  {'rs-bpe':>8s}  {'tiktoken':>8s}  {'c/rs':>6s}  {'c/tk':>6s}")
print("-" * 55)

for size in [100, 500, 1000, 2000, 5000, 10000, len(text_bytes)]:
    tb = text_bytes[:size]
    ts = text_str[:size]

    # warmup
    c_tok.count(tb); rs_tok.count(ts); tk_enc.encode(ts)

    n = max(20, 200000 // (size + 1))

    # c_bpe
    times = []
    for _ in range(n):
        t0 = time.perf_counter_ns()
        c_tok.count(tb)
        t1 = time.perf_counter_ns()
        times.append((t1 - t0) / 1e6)
    mc = statistics.median(times)

    # rs-bpe
    times = []
    for _ in range(n):
        t0 = time.perf_counter_ns()
        rs_tok.count(ts)
        t1 = time.perf_counter_ns()
        times.append((t1 - t0) / 1e6)
    mrs = statistics.median(times)

    # tiktoken
    times = []
    for _ in range(n):
        t0 = time.perf_counter_ns()
        tk_enc.encode(ts)
        t1 = time.perf_counter_ns()
        times.append((t1 - t0) / 1e6)
    mtk = statistics.median(times)

    print(f"{size:7d}B  {mc:7.3f}ms  {mrs:7.3f}ms  {mtk:7.3f}ms  {mc/mrs:5.2f}×  {mc/mtk:5.2f}×")
