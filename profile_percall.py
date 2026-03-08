"""Check if PCRE2 JIT is actually being used for our patterns."""
import ctypes, sys, os

# We can check via pcre2_pattern_info on the compiled pattern
# But easier: let's just measure the per-call overhead

import time, statistics
sys.path.insert(0, r"c:\Users\savov\source\forks\rs-bpe\c_bpe\python")
from c_bpe.openai import cl100k_base

tok = cl100k_base()

# Tiny text — measure per-call overhead
text = b"hello world this is a test"
tok.encode(text)

times = []
for _ in range(1000):
    t0 = time.perf_counter_ns()
    tok.count(text)
    t1 = time.perf_counter_ns()
    times.append(t1 - t0)
med = statistics.median(times)
print(f"count({len(text)}B): {med:.0f} ns = {med/1e3:.1f} µs")
# 6 pieces expected, so ~time/6 per match

text2 = b"a"
tok.encode(text2)
times = []
for _ in range(1000):
    t0 = time.perf_counter_ns()
    tok.count(text2)
    t1 = time.perf_counter_ns()
    times.append(t1 - t0)
med2 = statistics.median(times)
print(f"count(1B): {med2:.0f} ns = {med2/1e3:.1f} µs")

# Try 100 pieces
text3 = b"a " * 100
tok.encode(text3)
times = []
for _ in range(200):
    t0 = time.perf_counter_ns()
    tok.count(text3)
    t1 = time.perf_counter_ns()
    times.append(t1 - t0)
med3 = statistics.median(times)
npieces = tok.count(text3)
print(f"count(200B, ~100 pieces): {med3:.0f} ns = {med3/1e3:.1f} µs")
print(f"  per-piece: {med3/100:.0f} ns = {med3/100/1e3:.2f} µs")

# Now try the same with tiktoken
import tiktoken
enc = tiktoken.get_encoding("cl100k_base")

times_tk = []
for _ in range(200):
    t0 = time.perf_counter_ns()
    enc.encode("a " * 100)
    t1 = time.perf_counter_ns()
    times_tk.append(t1 - t0)
med_tk = statistics.median(times_tk)
print(f"\ntiktoken count(200B, ~100 pieces): {med_tk:.0f} ns = {med_tk/1e3:.1f} µs")
print(f"  per-piece: {med_tk/100:.0f} ns = {med_tk/100/1e3:.2f} µs")

print(f"\nRatio per-piece: {(med3/100)/(med_tk/100):.1f}×")
