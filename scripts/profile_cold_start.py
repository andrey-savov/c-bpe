"""Profile the cold-start breakdown for c_bpe and rs_bpe."""
import sys
import time

# Ensure totally cold
for k in list(sys.modules):
    if k.startswith(("c_bpe", "rs_bpe")):
        del sys.modules[k]

print("=== c_bpe cold start ===")
t0 = time.perf_counter()
import c_bpe.bpe  # noqa: E402
t1 = time.perf_counter()
tok_c = c_bpe.bpe.openai.cl100k_base()
t2 = time.perf_counter()
print(f"  import c_bpe.bpe:       {(t1-t0)*1000:8.1f} ms")
print(f"  openai.cl100k_base():   {(t2-t1)*1000:8.1f} ms")
print(f"  total:                  {(t2-t0)*1000:8.1f} ms")

# Now try o200k
t3 = time.perf_counter()
tok_o = c_bpe.bpe.openai.o200k_base()
t4 = time.perf_counter()
print(f"  openai.o200k_base():    {(t4-t3)*1000:8.1f} ms")

print()
# Clean and do rs_bpe
for k in list(sys.modules):
    if k.startswith("rs_bpe"):
        del sys.modules[k]

print("=== rs_bpe cold start ===")
t0 = time.perf_counter()
import rs_bpe.bpe  # noqa: E402
t1 = time.perf_counter()
tok_r = rs_bpe.bpe.openai.cl100k_base()
t2 = time.perf_counter()
print(f"  import rs_bpe.bpe:      {(t1-t0)*1000:8.1f} ms")
print(f"  openai.cl100k_base():   {(t2-t1)*1000:8.1f} ms")
print(f"  total:                  {(t2-t0)*1000:8.1f} ms")

t3 = time.perf_counter()
tok_o2 = rs_bpe.bpe.openai.o200k_base()
t4 = time.perf_counter()
print(f"  openai.o200k_base():    {(t4-t3)*1000:8.1f} ms")
