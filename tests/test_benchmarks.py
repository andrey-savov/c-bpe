"""
Benchmark tests comparing rs_bpe (Rust) and c_bpe (C) tokenizer implementations.

Uses pytest-benchmark to measure performance side-by-side.

Run with:
    pytest tests/test_benchmarks.py --benchmark-only
    pytest tests/test_benchmarks.py --benchmark-only --benchmark-sort=median
    pytest tests/test_benchmarks.py --benchmark-disable   # correctness only
    pytest tests/test_benchmarks.py --benchmark-only -k rs_bpe   # one impl only
"""

import importlib
import sys

import pytest


# ---------------------------------------------------------------------------
# Import-time benchmarks — measure module load + tokenizer construction
# ---------------------------------------------------------------------------

def _fresh_import_and_construct(pkg: str, factory: str):
    """Import the module from scratch and call the tokenizer factory."""
    # Remove cached modules so the import is measured from scratch
    to_remove = [k for k in sys.modules if k.startswith(pkg)]
    for k in to_remove:
        del sys.modules[k]
    mod = importlib.import_module(f"{pkg}.bpe")
    return getattr(mod.openai, factory)()


@pytest.mark.parametrize(
    "impl,pkg,factory",
    [
        pytest.param("rs_bpe", "rs_bpe", "cl100k_base", id="rs_bpe"),
        pytest.param("c_bpe", "c_bpe", "cl100k_base", id="c_bpe"),
    ],
)
def test_import_cl100k(benchmark, impl, pkg, factory):
    """Benchmark import + cl100k_base construction time (cold import)."""
    try:
        tok = benchmark(_fresh_import_and_construct, pkg, factory)
        assert tok is not None
    except ImportError:
        pytest.skip(f"{impl} not installed")


@pytest.mark.parametrize(
    "impl,pkg,factory",
    [
        pytest.param("rs_bpe", "rs_bpe", "o200k_base", id="rs_bpe"),
        pytest.param("c_bpe", "c_bpe", "o200k_base", id="c_bpe"),
    ],
)
def test_import_o200k(benchmark, impl, pkg, factory):
    """Benchmark import + o200k_base construction time (cold import)."""
    try:
        tok = benchmark(_fresh_import_and_construct, pkg, factory)
        assert tok is not None
    except ImportError:
        pytest.skip(f"{impl} not installed")


# ---------------------------------------------------------------------------
# Text fixtures
# ---------------------------------------------------------------------------

SMALL_TEXT = "This is a small test string for tokenization."

_README_PATH = "README.md"
try:
    with open(_README_PATH, "r", encoding="utf-8") as _f:
        MEDIUM_TEXT = _f.read()
except FileNotFoundError:
    MEDIUM_TEXT = SMALL_TEXT * 20

LARGE_TEXT = MEDIUM_TEXT * 50

TEXT_PARAMS = [
    pytest.param("small",  SMALL_TEXT,  id="small"),
    pytest.param("medium", MEDIUM_TEXT, id="medium"),
    pytest.param("large",  LARGE_TEXT,  id="large"),
]

_BATCH_SMALL  = [SMALL_TEXT]  * 100
_BATCH_MEDIUM = [MEDIUM_TEXT] * 10
_BATCH_LARGE  = [LARGE_TEXT]  * 2


# ---------------------------------------------------------------------------
# Tokenizer fixtures — parametrized over both implementations
# ---------------------------------------------------------------------------

@pytest.fixture(scope="module", params=["rs_bpe", "c_bpe"])
def cl100k(request):
    """cl100k_base tokenizer — runs once for rs_bpe and once for c_bpe."""
    if request.param == "rs_bpe":
        try:
            from rs_bpe.bpe import openai
            return openai.cl100k_base()
        except ImportError:
            pytest.skip("rs_bpe not installed")
    else:
        try:
            from c_bpe.bpe import openai
            return openai.cl100k_base()
        except ImportError:
            pytest.skip("c_bpe not installed")


@pytest.fixture(scope="module", params=["rs_bpe", "c_bpe"])
def o200k(request):
    """o200k_base tokenizer — runs once for rs_bpe and once for c_bpe."""
    if request.param == "rs_bpe":
        try:
            from rs_bpe.bpe import openai
            return openai.o200k_base()
        except ImportError:
            pytest.skip("rs_bpe not installed")
    else:
        try:
            from c_bpe.bpe import openai
            return openai.o200k_base()
        except ImportError:
            pytest.skip("c_bpe not installed")


# ---------------------------------------------------------------------------
# Encoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_encode_cl100k(benchmark, cl100k, label, text):
    """Benchmark encoding throughput for cl100k_base across text sizes."""
    result = benchmark(cl100k.encode, text)
    assert isinstance(result, list) and len(result) > 0
    benchmark.extra_info["tokens"] = len(result)


# ---------------------------------------------------------------------------
# Decoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_decode_cl100k(benchmark, cl100k, label, text):
    """Benchmark decoding for cl100k_base. Tokens pre-encoded outside the timed loop."""
    tokens = cl100k.encode(text)  # setup — not benchmarked
    benchmark.extra_info["tokens"] = len(tokens)
    result = benchmark(cl100k.decode, tokens)
    assert result == text


# ---------------------------------------------------------------------------
# Decoding benchmarks – o200k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_decode_o200k(benchmark, o200k, label, text):
    """Benchmark decoding for o200k_base. Tokens pre-encoded outside the timed loop."""
    tokens = o200k.encode(text)  # setup — not benchmarked
    benchmark.extra_info["tokens"] = len(tokens)
    result = benchmark(o200k.decode, tokens)
    assert result == text


# ---------------------------------------------------------------------------
# Roundtrip benchmarks – cl100k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_roundtrip_cl100k(benchmark, cl100k, label, text):
    """Benchmark encode → decode roundtrip for cl100k_base."""
    benchmark.extra_info["tokens"] = len(cl100k.encode(text))

    def roundtrip(t):
        return cl100k.decode(cl100k.encode(t))

    result = benchmark(roundtrip, text)
    assert result == text


# ---------------------------------------------------------------------------
# Token counting benchmarks
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_count_cl100k(benchmark, cl100k, label, text):
    """Benchmark token counting for cl100k_base. count() must agree with len(encode())."""
    count = benchmark(cl100k.count, text)
    assert count == len(cl100k.encode(text))
    benchmark.extra_info["tokens"] = count


# ---------------------------------------------------------------------------
# count_till_limit benchmarks
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_count_till_limit_cl100k(benchmark, cl100k, label, text):
    """Benchmark count_till_limit (limit = 50 tokens)."""
    limit = 50
    benchmark.extra_info["tokens"] = min(len(cl100k.encode(text)), limit)
    result = benchmark(cl100k.count_till_limit, text, limit)
    assert result is None or isinstance(result, int)


# ---------------------------------------------------------------------------
# Batch encoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize(
    "label,batch",
    [
        pytest.param("small_x100",  _BATCH_SMALL,  id="small_x100"),
        pytest.param("medium_x10",  _BATCH_MEDIUM, id="medium_x10"),
        pytest.param("large_x2",    _BATCH_LARGE,  id="large_x2"),
    ],
)
def test_encode_batch_cl100k(benchmark, cl100k, label, batch):
    """Benchmark batch encoding for cl100k_base.

    encode_batch returns (list[list[int]], total_token_count, time_taken).
    """
    tokens_list, total_tokens, _elapsed = benchmark(cl100k.encode_batch, batch)
    assert total_tokens > 0
    assert len(tokens_list) == len(batch)
    benchmark.extra_info["tokens"] = total_tokens


# ---------------------------------------------------------------------------
# Parallel batch encoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------

@pytest.mark.parametrize(
    "label,batch",
    [
        pytest.param("small_x100",  _BATCH_SMALL,  id="small_x100"),
        pytest.param("medium_x10",  _BATCH_MEDIUM, id="medium_x10"),
        pytest.param("large_x2",    _BATCH_LARGE,  id="large_x2"),
    ],
)
def test_encode_batch_parallel_cl100k(benchmark, cl100k, label, batch):
    """Benchmark parallel batch encoding for cl100k_base.

    encode_batch_parallel returns (list[list[int]], total_tokens, time_taken, threads_used).

    Uses pedantic mode with warmup disabled to avoid crashing the c_bpe C11
    thread pool on MSVC (rapid-fire warmup iterations trigger an access
    violation in MSVC's <threads.h> implementation).
    """
    tokens_list, total_tokens, _elapsed, _threads = benchmark.pedantic(
        cl100k.encode_batch_parallel, args=(batch, None),
        rounds=5, warmup_rounds=0)
    assert total_tokens > 0
    assert len(tokens_list) == len(batch)
    benchmark.extra_info["tokens"] = total_tokens


# ---------------------------------------------------------------------------
# o200k_base encoding / roundtrip
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_encode_o200k(benchmark, o200k, label, text):
    """Benchmark encoding throughput for o200k_base across text sizes."""
    result = benchmark(o200k.encode, text)
    assert isinstance(result, list) and len(result) > 0
    benchmark.extra_info["tokens"] = len(result)


@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_roundtrip_o200k(benchmark, o200k, label, text):
    """Benchmark encode → decode roundtrip for o200k_base."""
    benchmark.extra_info["tokens"] = len(o200k.encode(text))

    def roundtrip(t):
        return o200k.decode(o200k.encode(t))

    result = benchmark(roundtrip, text)
    assert result == text
