"""Quick profile of c_bpe encoding on medium text."""
import time, statistics

with open("README.md", "r") as f:
    MEDIUM_TEXT = f.read()

b = MEDIUM_TEXT.encode("utf-8")
print(f"Medium text: {len(MEDIUM_TEXT)} chars, {len(b)} bytes")

from c_bpe.bpe.openai import cl100k_base
t = cl100k_base()
bpe = t.bpe()

# Tokenizer.encode (regex split + backtracking)
t.encode(b)  # warm
times = []
for _ in range(5):
    t0 = time.perf_counter()
    tokens = t.encode(b)
    dt = time.perf_counter() - t0
    times.append(dt)
print(f"  tokenizer.encode: {statistics.median(times)*1000:.3f} ms ({len(tokens)} tokens)")

# Compare with tiktoken
try:
    import tiktoken
    tt = tiktoken.get_encoding("cl100k_base")
    tt.encode(MEDIUM_TEXT)  # warm
    times3 = []
    for _ in range(5):
        t0 = time.perf_counter()
        tokens3 = tt.encode(MEDIUM_TEXT)
        dt = time.perf_counter() - t0
        times3.append(dt)
    print(f"  tiktoken:         {statistics.median(times3)*1000:.3f} ms ({len(tokens3)} tokens)")
except ImportError:
    print("  tiktoken not available")

# Profile per-piece: how many pieces does regex produce and how big are they?
print("\n--- Regex piece analysis ---")
# We need to call the C pretokenizer. Let's measure by encoding short texts.
# But we can approximate: tokenizer.encode produces 3435 tokens from regex pieces
# while raw backtracking produces 3455 tokens (slightly different because no regex split).
# The issue is: how much time is regex vs BPE per piece?

# Time just the count() method which also uses backtracking
t.count(b)  # warm
times_c = []
for _ in range(5):
    t0 = time.perf_counter()
    c = t.count(b)
    dt = time.perf_counter() - t0
    times_c.append(dt)
print(f"  tokenizer.count:  {statistics.median(times_c)*1000:.3f} ms (count={c})")

# Time encoding small pieces to see per-token overhead
pieces = MEDIUM_TEXT.split()  # word-level split
encoded_pieces = [p.encode("utf-8") for p in pieces]
t.encode(encoded_pieces[0])  # warm
t0 = time.perf_counter()
for p in encoded_pieces:
    t.encode(p)
dt = time.perf_counter() - t0
print(f"  {len(pieces)} word-level pieces: {dt*1000:.3f} ms total, {dt*1000/len(pieces):.3f} ms/piece")

# Profile ac_leftmost_longest via bpe.count on raw bytes
bpe.count(b)  # warm
times_bpe_c = []
for _ in range(5):
    t0 = time.perf_counter()
    c2 = bpe.count(b)
    dt = time.perf_counter() - t0
    times_bpe_c.append(dt)
print(f"\n  bpe.count (raw):  {statistics.median(times_bpe_c)*1000:.3f} ms (count={c2})")

# Profile bpe.encode on small chunks to see scaling
for size in [100, 500, 1000, 5000, 15000]:
    chunk = b[:size]
    bpe.encode_via_backtracking(chunk)  # warm
    t0 = time.perf_counter()
    for _ in range(3):
        bpe.encode_via_backtracking(chunk)
    dt = (time.perf_counter() - t0) / 3
    toks = bpe.encode_via_backtracking(chunk)
    print(f"  bpe.encode_via_backtracking({size:5d}B): {dt*1000:.3f} ms ({len(toks)} tokens)")

# Profile: regex splitting vs per-piece encoding 
# Simulate what tokenizer.encode does: split, then encode each piece
print("\n--- Regex vs BPE breakdown ---")

# We can't call the C regex directly, but we can time encoding random chunks
# that match typical regex piece sizes
import re
# cl100k regex pattern (Python approximation)
pat = re.compile(r"""(?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\r\n\p{L}\p{N}]?\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]+[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+""")
# Just use simple whitespace split as proxy
words = MEDIUM_TEXT.split()
pieces = [w.encode("utf-8") for w in words]
piece_sizes = [len(p) for p in pieces]
print(f"  ~{len(pieces)} pieces, avg size {sum(piece_sizes)/len(piece_sizes):.1f}B, max {max(piece_sizes)}B")

# Time encoding all pieces via raw BPE
for p in pieces[:10]:
    bpe.encode_via_backtracking(p)
t0 = time.perf_counter()
for p in pieces:
    bpe.encode_via_backtracking(p)
dt_pieces = time.perf_counter() - t0
print(f"  BPE encode {len(pieces)} word-pieces: {dt_pieces*1000:.3f} ms")
print(f"  => regex overhead estimate: {22 - dt_pieces*1000:.1f} ms (tokenizer - BPE pieces)")
