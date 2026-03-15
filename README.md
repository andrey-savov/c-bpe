[![CI](https://github.com/andrey-savov/c-bpe/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/andrey-savov/c-bpe/actions/workflows/ci.yml)

# c-bpe

High-performance C implementation of BPE (Byte Pair Encoding) tokenizer with Python bindings.

This library provides fast and correct token counting for chunking algorithms with a focus on high performance. It implements novel algorithms for BPE tokenization that are both correct and significantly faster than existing solutions.

## Attribution

This project is based on [rs-bpe](https://github.com/gweidart/rs-bpe) by [gweidart](https://github.com/gweidart), a Rust implementation of BPE tokenization. The C implementation ports the same novel algorithms to pure C for maximum portability and performance. The original Rust implementation is included in the [`rust/`](rust/) directory for comparison benchmarking.

#### Installation

```
pip install c-bpe
```

*c_bpe consistently outperforms tiktoken (March 7, 2026)*

![c_bpe throughput vs tiktoken](benchmark/tokenizer_benchmark_results_throughput.svg)

## Key Features

* Efficient token counting with linear time complexity even for adversarial inputs
* Split text at exact token boundaries while respecting UTF-8 character boundaries
* Incrementally count tokens while appending text to a chunk
* Calculate token counts for sub-ranges of text with constant-time complexity
* Python bindings with OpenAI-compatible interface

These operations are particularly important for LLM applications but are challenging to implement efficiently for BPE tokenization.

## Motivation *(problems this library aims to solve)*

Existing BPE tokenizers often face performance and correctness issues when used for chunking operations:

### Split-at-N-Tokens Problem

Naively splitting text after N tokens by first encoding the entire text and then selecting a boundary often produces suboptimal results:

* The split point might not align with a UTF-8 character boundary
* Dropping tokens until a character boundary is reached might result in chunks much shorter than desired
* The algorithm wastes resources by encoding more text than necessary

### Incremental Counting Problem

Incrementally counting tokens as text is appended is challenging with traditional implementations:

* Recomputing the encoding after every append leads to quadratic complexity
* Approximating counts by aggregating piece counts leads to incorrect results due to BPE's non-monotonic nature
* Incorrect counting can cause problems when staying within token limits for LLM APIs

### Interval Counting Problem

Counting tokens for arbitrary subranges traditionally requires reprocessing the entire substring:

* Leads to poor performance for applications that need to count many subranges
* Makes operations like binary search for token boundaries inefficient

Our library provides novel algorithms to solve these problems with superior performance characteristics.

## Implementation

### Core Algorithm

The novel O(n) algorithm preserves the exact output of the original BPE algorithm by tracking encodings of all text prefixes using mathematical properties of valid BPE encodings.

Instead of storing full token sequences for each prefix, only the last token of each prefix needs to be remembered. This is possible because:

1. There exists exactly one valid encoding sequence for any input text
2. Any substring of a valid encoding sequence is itself a valid encoding sequence
3. Knowing the last token of a valid encoding sequence uniquely determines the full sequence

The algorithm determines the correct last token for each prefix by checking token compatibility with the preceding token, yielding a linear-time solution.

### Backtracking Optimization

For average-case improvement, a backtracking-based algorithm:

1. Tries the greedy approach first, using the longest matching token at each step
2. Backtracks when necessary to produce a valid BPE encoding
3. Uses a bitfield so worst-case runtime stays linear in input length

### Data Structures

* **`BytePairEncoding` struct**: Stores the concatenated token byte array, per-token start offsets, a `BytesMap` (bytes→token id), `split_left`/`split_right` arrays for token decomposition, a `PairMap` (pair→merged token), three Aho-Corasick automatons, and a `next_prefix_match` table.

* **`PairMap`**: Open-addressing hash table (linear probing, 50% max load) for `(token1, token2) → merged_id` lookups. Uses a splitmix64 finaliser instead of byte-by-byte FNV-1a for the fixed 8-byte key, keeping the merge step cache-friendly.

* **`BytesMap`**: Open-addressing hash table for `bytes → token_id` lookups. Uses FNV-1a hashing identical to Rust's `fnv` crate, ensuring consistent hash values across both implementations.

### Aho-Corasick Automatons

Three Double-Array Aho-Corasick automatons are built over the token vocabulary at initialisation time:

* **`longest_searcher`** (`AC_KIND_LEFTMOST_LONGEST`): leftmost-longest token match at each position — used for the backtrack encoder.
* **`overlapping_searcher`** (`AC_KIND_OVERLAPPING_FWD`): all overlapping forward matches — used by `AppendableEncoder` to maintain per-byte AC state.
* **`overlapping_searcher_rev`** (`AC_KIND_OVERLAPPING_REV`): all overlapping reverse matches — used by `PrependableEncoder`.

The Double-Array layout gives O(1) state transitions per input byte, making the automaton traversal extremely cache-friendly.

### Special Purpose Encoders

* **`AppendableEncoder`**: Stores one `AppState` per byte appended (`ac_state`, `last_token`, running count), allowing O(1) amortised count queries via the forward AC automaton.
* **`PrependableEncoder`**: Mirror of `AppendableEncoder` using the reverse AC automaton — supports O(1) amortised queries while prepending.
* **`IntervalEncoding`**: Precomputes a `last_token`, `tree_id`, `tree_end`, and `tree_depth` array per byte position, enabling typically-O(1) `count(start, end)` queries.
* **OpenAI-compatible Tokenizer**: Hand-coded pre-tokenisation with PCRE2 UCD tables (regex splitting identical to tiktoken) feeding into the shared BPE encode/decode logic.

## Performance

Our benchmarks show significant performance improvements over existing implementations:

> **Note**: All benchmark results shown here were achieved using the Python bindings, not the direct native implementation. This provides a more realistic representation of the performance users will experience in Python applications.

### Single-Text Tokenization

| Text Size | c\_bpe vs tiktoken | rs\_bpe vs tiktoken |
| ----------- | ----------------------- | -------------------- |
| Small     | 2.9× faster           | 3.0× faster        |
| Medium    | 1.7× faster           | 1.6× faster        |
| Large     | 4.4× faster           | 2.3× faster        |

_Encoding speed (benchmark.py results):_

![Encoding throughput](assets/20260307_tokenizer_benchmark_results_throughput.svg)

```
SMALL TEXT:
  tiktoken: 0.000102s
  c_bpe:    0.000035s
  rs_bpe:   0.000034s

MEDIUM TEXT:
  tiktoken: 0.001735s
  c_bpe:    0.001007s
  rs_bpe:   0.001092s

LARGE TEXT:
  tiktoken: 0.068093s
  c_bpe:    0.015330s
  rs_bpe:   0.029147s
```

Both libraries also provide significantly faster decoding and roundtrip operations:

_Decoding speed:_

![Tokenizer timing comparison](assets/20260307_tokenizer_benchmark_results_time.svg)

```
SMALL TEXT:
  tiktoken: 0.000027s
  c_bpe:    0.000011s
  rs_bpe:   0.000018s

MEDIUM TEXT:
  tiktoken: 0.000200s
  c_bpe:    0.000076s
  rs_bpe:   0.000105s

LARGE TEXT:
  tiktoken: 0.003799s
  c_bpe:    0.001709s
  rs_bpe:   0.002504s
```

### Batch Processing Performance

| Batch Size | c\_bpe encode | c\_bpe decode | rs\_bpe encode | rs\_bpe decode |
| ------------ | ------------- | ------------- | -------------- | -------------- |
| 1          | 35× faster  | 165× faster | 79× faster   | 94× faster   |
| 10         | 32× faster  | 92× faster  | 43× faster   | 100× faster  |
| 100        | 5× faster   | 94× faster  | 17× faster   | 52× faster   |
| 1000       | 22× faster  | 57× faster  | 13× faster   | 31× faster   |

_Encode speedup vs tiktoken (all sizes):_

![Encode speedup vs tiktoken](assets/20260307_tokenizer_benchmark_results_speedup.svg)

### Worst-Case Performance

While tiktoken shows quadratic growth for certain adversarial inputs, c_bpe maintains linear scaling even in worst-case scenarios. This is critical for production systems that need consistent performance guarantees.

### Key Performance Advantages

1. **Memory Efficiency**: Compact data structures (tightly-packed token byte arrays, power-of-2 hash tables at ≤50% load) and no redundant token storage
2. **Cache-Friendly Hash Tables**: `PairMap` uses a splitmix64 finaliser for fixed 8-byte keys; `BytesMap` uses FNV-1a — both with linear probing for sequential memory access
3. **O(1) State Transitions**: Double-Array Aho-Corasick automatons enable single-byte-per-step token matching without backtracking through the vocabulary
4. **Full LTO**: Compiled with full Link-Time Optimisation (MSVC `/GL`+`/LTCG` / GCC `-flto`)
5. **No Correctness Trade-offs**: Verified to produce token-for-token identical output to tiktoken

All benchmarks were run on standard hardware and results may vary based on your specific environment.

## Python Usage Examples

### Basic Tokenization

```python
from c_bpe.bpe import openai

# Load OpenAI tokenizers (automatically caches for reuse)
cl100k_tokenizer = openai.cl100k_base()  # GPT-3.5/4 tokenizer
o200k_tokenizer = openai.o200k_base()    # o200k tokenizer

# Basic encoding
text = "Hello, world! This is an example."
tokens = cl100k_tokenizer.encode(text)
print(f"Encoded tokens: {tokens}")

# Basic decoding
decoded_text = cl100k_tokenizer.decode(tokens)
print(f"Decoded text: {decoded_text}")

# Simple token counting
token_count = cl100k_tokenizer.count(text)
print(f"Token count: {token_count}")
```

### Efficient Token Limiting

One of the key features is the ability to efficiently count tokens up to a limit, which is useful when you need to stay within token constraints:

```python
from c_bpe.bpe import openai

tokenizer = openai.cl100k_base()
max_tokens = 50

# Count tokens until limit is reached
text = "This is a long text that might exceed our token limit... " * 20
char_position = tokenizer.count_till_limit(text, max_tokens)

if char_position is not None:
    # We reached the limit before the end of the text
    truncated_text = text[:char_position]
    print(f"Truncated to {tokenizer.count(truncated_text)} tokens")
    print(f"Truncated text: {truncated_text}")
else:
    # The entire text is within our token limit
    print(f"Text is within token limit: {tokenizer.count(text)} tokens")
```

### Batch Processing

c_bpe excels at batch processing, which is perfect for processing large datasets:

```python
from c_bpe.bpe import openai
import time

# Load the tokenizer
tokenizer = openai.cl100k_base()

# Create a batch of texts
texts = [
    "This is the first document to encode.",
    "Here's another one with different content.",
    "A third document with some more text to process.",
    # Add more as needed...
]

# Configure parallel processing options (optional)
parallel_options = openai.ParallelOptions(
    min_batch_size=20,      # Minimum batch size to engage parallel processing
    chunk_size=100,         # Number of texts to process in each thread
    max_threads=0,          # 0 means use optimal thread count (based on CPU cores)
    use_thread_pool=True    # Reuse thread pool for better performance
)

# Encode batch with performance metrics
start_time = time.time()
result = tokenizer.encode_batch(texts, parallel_options)
end_time = time.time()

print(f"Processed {len(texts)} texts in {result.time_taken:.6f}s")
print(f"Total tokens: {result.total_tokens}")
print(f"Throughput: {result.total_tokens / result.time_taken:.1f} tokens/second")

# Access individual token lists
for i, tokens in enumerate(result.tokens):
    print(f"Text {i} has {len(tokens)} tokens")
```

### Text Chunking

c_bpe can be used to efficiently chunk text based on token counts:

```python
from c_bpe.bpe import openai

tokenizer = openai.cl100k_base()

def chunk_text(text, max_chunk_tokens=1024, overlap_tokens=50):
    """Split text into chunks of approximately max_chunk_tokens."""
    chunks = []

    # Get the full text token count
    total_tokens = tokenizer.count(text)

    if total_tokens <= max_chunk_tokens:
        return [text]

    # Keep track of where we are in the text
    start_pos = 0

    while start_pos < len(text):
        # Find where to end this chunk
        char_position = tokenizer.count_till_limit(text[start_pos:], max_chunk_tokens)

        if char_position is None:
            # The rest of the text fits within our limit
            chunks.append(text[start_pos:])
            break

        # Add the chunk
        end_pos = start_pos + char_position
        chunks.append(text[start_pos:end_pos])

        # Move to the next chunk, considering overlap
        if overlap_tokens > 0 and end_pos < len(text):
            # Move back by overlap tokens
            overlap_char_position = tokenizer.count_till_limit(
                text[start_pos:end_pos], max_chunk_tokens - overlap_tokens
            )
            if overlap_char_position is not None:
                start_pos += overlap_char_position
            else:
                start_pos = end_pos
        else:
            start_pos = end_pos

    return chunks

# Example usage
long_text = "This is a long document that needs to be split into chunks. " * 100
chunks = chunk_text(long_text, max_chunk_tokens=100, overlap_tokens=10)

print(f"Split text into {len(chunks)} chunks:")
for i, chunk in enumerate(chunks):
    token_count = tokenizer.count(chunk)
    print(f"Chunk {i}: {token_count} tokens, {len(chunk)} chars")
```

## Building from Source

### Prerequisites

| Requirement | c_bpe | rs_bpe (companion) |
|-------------|-------|---------------------|
| Python ≥ 3.9 | ✅ | ✅ |
| C11 compiler (GCC, Clang, or MSVC) | ✅ | — |
| Rust toolchain (stable) | — | ✅ |
| setuptools (`pip install setuptools`) | ✅ | — |
| maturin (`pip install maturin`) | — | ✅ |

### c_bpe (C extension)

```bash
# Clone the repository
git clone https://github.com/andrey-savov/c-bpe.git
cd c-bpe

# Create and activate a virtual environment (recommended)
python -m venv .venv
source .venv/bin/activate   # Linux/macOS
# .venv\Scripts\activate    # Windows

# Install in development mode (editable)
pip install -e .

# Or build the extension in-place without installing
python setup.py build_ext --inplace
```

The build auto-detects the compiler and applies platform-appropriate optimisations:
- **GCC/Clang**: `-O3 -march=native -flto -DNDEBUG`
- **MSVC**: `/O2 /Ox /GL /DNDEBUG` with `/LTCG` at link time

PCRE2 Unicode tables are bundled in `third_party/`; no external PCRE2 installation is required.

### rs_bpe (Rust companion — for benchmarking)

The `rust/` directory contains the original Rust implementation ([rs-bpe](https://github.com/gweidart/rs-bpe)) for comparison benchmarking:

```bash
cd rust

# Install maturin if not already present
pip install maturin

# Build and install in development mode
maturin develop --release
```

The Rust toolchain can be installed from [rustup.rs](https://rustup.rs/).

### Installing both side by side

Both packages can be installed in the same environment — they use separate namespaces (`c_bpe` and `rs_bpe`):

```bash
cd c-bpe
pip install -e .                                    # c_bpe
cd rust && pip install maturin && maturin develop --release   # rs_bpe
```

Verify:

```python
from c_bpe.bpe import openai as c_openai
from rs_bpe.bpe import openai as rs_openai

c_tok  = c_openai.cl100k_base()
rs_tok = rs_openai.cl100k_base()

text = "Hello, world!"
assert c_tok.encode(text) == rs_tok.encode(text)
print("Both implementations installed and producing identical output.")
```

### Running tests

```bash
# From the repository root, with both implementations installed:
pip install pytest pytest-benchmark

# Correctness tests
pytest tests/test_basic.py -v

# Benchmarks
pytest tests/test_benchmarks.py --benchmark-only -v
```

## Acknowledgements

This project was developed with the assistance of [Claude Code](https://claude.ai/claude-code), using Claude Opus 4.6 and Claude Sonnet 4.6 models by [Anthropic](https://www.anthropic.com).

## License

[MIT License](LICENSE)
