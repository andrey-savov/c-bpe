import sys, time
sys.stdout.write('starting...\n')
sys.stdout.flush()
t0 = time.time()
from c_bpe.bpe.openai import cl100k_base
sys.stdout.write(f'imported in {time.time()-t0:.1f}s\n')
sys.stdout.flush()
t0 = time.time()
t = cl100k_base()
sys.stdout.write(f'built in {time.time()-t0:.1f}s\n')
sys.stdout.flush()

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
