"""
Benchmark tests for rs_bpe encoding and decoding speed.

Uses pytest-benchmark to measure performance of:
- Encoding (text → tokens)
- Decoding (tokens → text)
- Roundtrip (encode then decode)
- Token counting
- Batch encoding

Text sizes mirror the definitions used in benchmark/benchmark.py.
Run with:
    pytest tests/test_benchmarks.py --benchmark-only
    pytest tests/test_benchmarks.py --benchmark-only --benchmark-sort=mean
    pytest tests/test_benchmarks.py --benchmark-only --benchmark-compare
    pytest tests/test_benchmarks.py --benchmark-disable   # correctness only
"""

import pytest

try:
    import rs_bpe
    from rs_bpe.bpe import openai
except ImportError:
    pytest.skip("rs_bpe package not installed", allow_module_level=True)


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


# ---------------------------------------------------------------------------
# Tokenizer fixtures
# ---------------------------------------------------------------------------


@pytest.fixture(scope="module")
def cl100k():
    """Cache the cl100k_base tokenizer for the entire module."""
    return openai.cl100k_base()


@pytest.fixture(scope="module")
def o200k():
    """Cache the o200k_base tokenizer for the entire module."""
    return openai.o200k_base()


# Pre-encoded token sequences used by decode benchmarks.
# Computed once at module import so decode timings exclude encoding overhead.
_TEXT_MAP: dict[str, str] = {
    "small":  SMALL_TEXT,
    "medium": MEDIUM_TEXT,
    "large":  LARGE_TEXT,
}

def _precompute_tokens(
    tokenizer_factory,
) -> dict[str, list[int]]:
    tok = tokenizer_factory()
    return {label: tok.encode(text) for label, text in _TEXT_MAP.items()}


_CL100K_TOKENS: dict[str, list[int]] = _precompute_tokens(openai.cl100k_base)
_O200K_TOKENS:  dict[str, list[int]] = _precompute_tokens(openai.o200k_base)

DECODE_PARAMS_CL100K = [
    pytest.param(label, _CL100K_TOKENS[label], _TEXT_MAP[label], id=label)
    for label in ("small", "medium", "large")
]
DECODE_PARAMS_O200K = [
    pytest.param(label, _O200K_TOKENS[label], _TEXT_MAP[label], id=label)
    for label in ("small", "medium", "large")
]


# ---------------------------------------------------------------------------
# Encoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_encode_cl100k(benchmark, cl100k, label, text):
    """Benchmark encoding throughput for cl100k_base across text sizes."""
    result = benchmark(cl100k.encode, text)
    assert isinstance(result, list)
    assert len(result) > 0
    benchmark.extra_info["tokens"] = len(result)


# ---------------------------------------------------------------------------
# Decoding benchmarks – cl100k_base
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("label,tokens,expected", DECODE_PARAMS_CL100K)
def test_decode_cl100k(benchmark, cl100k, label, tokens, expected):
    """Benchmark decoding for cl100k_base across text sizes.

    Token lists are pre-encoded at module import so only decode time is measured.
    """
    benchmark.extra_info["tokens"] = len(tokens)
    result = benchmark(cl100k.decode, tokens)
    assert result == expected


# ---------------------------------------------------------------------------
# Decoding benchmarks – o200k_base
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("label,tokens,expected", DECODE_PARAMS_O200K)
def test_decode_o200k(benchmark, o200k, label, tokens, expected):
    """Benchmark decoding for o200k_base across text sizes.

    Token lists are pre-encoded at module import so only decode time is measured.
    """
    benchmark.extra_info["tokens"] = len(tokens)
    result = benchmark(o200k.decode, tokens)
    assert result == expected


# ---------------------------------------------------------------------------
# Roundtrip benchmarks – cl100k_base
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_roundtrip_cl100k(benchmark, cl100k, label, text):
    """Benchmark encode → decode roundtrip for cl100k_base."""
    benchmark.extra_info["tokens"] = len(_CL100K_TOKENS[label])

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
    """Benchmark count_till_limit for a limit of 50 tokens.

    Stops as soon as 50 tokens are found, so tokens processed = min(total, 50).
    """
    limit = 50
    benchmark.extra_info["tokens"] = min(len(_CL100K_TOKENS[label]), limit)
    result = benchmark(cl100k.count_till_limit, text, limit)
    # Result is a char position (int) when limit exceeded, or None if text fits
    assert result is None or isinstance(result, int)


# ---------------------------------------------------------------------------
# Batch encoding benchmarks
# ---------------------------------------------------------------------------

_BATCH_SMALL = [SMALL_TEXT] * 100
_BATCH_MEDIUM = [MEDIUM_TEXT] * 10
_BATCH_LARGE = [LARGE_TEXT] * 2

@pytest.mark.parametrize(
    "label,batch",
    [
        pytest.param("small_x100",  _BATCH_SMALL,  id="small_x100"),
        pytest.param("medium_x10",  _BATCH_MEDIUM, id="medium_x10"),
        pytest.param("large_x2",    _BATCH_LARGE,  id="large_x2"),
    ],
)
def test_encode_batch_cl100k(benchmark, cl100k, label, batch):
    """Benchmark parallel batch encoding for cl100k_base.

    encode_batch returns (list[list[int]], total_token_count, time_taken).
    """
    tokens_list, total_tokens, _elapsed = benchmark(cl100k.encode_batch, batch)
    assert total_tokens > 0
    assert len(tokens_list) == len(batch)
    benchmark.extra_info["tokens"] = total_tokens


# ---------------------------------------------------------------------------
# o200k_base encoding / decoding
# ---------------------------------------------------------------------------


@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_encode_o200k(benchmark, o200k, label, text):
    """Benchmark encoding throughput for o200k_base across text sizes."""
    result = benchmark(o200k.encode, text)
    assert isinstance(result, list)
    assert len(result) > 0
    benchmark.extra_info["tokens"] = len(result)


@pytest.mark.parametrize("label,text", TEXT_PARAMS)
def test_roundtrip_o200k(benchmark, o200k, label, text):
    """Benchmark encode → decode roundtrip for o200k_base."""
    benchmark.extra_info["tokens"] = len(_O200K_TOKENS[label])

    def roundtrip(t):
        return o200k.decode(o200k.encode(t))

    result = benchmark(roundtrip, text)
    assert result == text
