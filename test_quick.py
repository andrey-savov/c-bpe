"""Quick profiling: batch vs N×single to isolate overhead."""
import time

from c_bpe.bpe import openai as c_openai
c_tok = c_openai.cl100k_base()
from rs_bpe.bpe import openai as r_openai
r_tok = r_openai.cl100k_base()

with open("README.md") as f:
    MEDIUM = f.read()

SMALL = "This is a small test string for tokenization."
batch_small = [SMALL] * 100
batch_medium = [MEDIUM] * 10
REPS = 500


def bench(tok, fn, label):
    times = []
    for _ in range(REPS):
        t0 = time.perf_counter_ns()
        fn(tok)
        times.append(time.perf_counter_ns() - t0)
    times.sort()
    med = times[len(times) // 2]
    mn = times[0]
    print(f"  {label:40s}  median={med/1e3:10.1f} us  min={mn/1e3:.1f} us")


print("=== small x 100 ===")
bench(c_tok, lambda t: t.encode_batch(batch_small),  "c_bpe  batch")
bench(c_tok, lambda t: [t.encode(x) for x in batch_small], "c_bpe  N*single")
bench(r_tok, lambda t: t.encode_batch(batch_small),  "rs_bpe batch")
bench(r_tok, lambda t: [t.encode(x) for x in batch_small], "rs_bpe N*single")

print("\n=== medium x 10 ===")
bench(c_tok, lambda t: t.encode_batch(batch_medium), "c_bpe  batch")
bench(c_tok, lambda t: [t.encode(x) for x in batch_medium], "c_bpe  N*single")
bench(r_tok, lambda t: t.encode_batch(batch_medium), "rs_bpe batch")
bench(r_tok, lambda t: [t.encode(x) for x in batch_medium], "rs_bpe N*single")

raise SystemExit(0)

tests = [
    (b'!', [0]),
    (b'"', [1]),
    (b'a', [64]),
    (b' world', [1917]),
    (b'hello', [15339]),
    (b'hello world', [15339, 1917]),
]
for inp, expected in tests:
    got = t.encode(inp)
    status = 'OK' if got == expected else 'FAIL'
    sys.stdout.write(f'{status}: encode({inp}) = {got}  (expected {expected})\n')
    sys.stdout.flush()
