"""
Basic tests for the c_bpe package.

These tests mirror the rs_bpe test suite to verify drop-in compatibility.
Run with: pytest c_bpe/tests/test_basic.py -v
"""

import pytest

try:
    import c_bpe
    from c_bpe import openai
    from c_bpe.bpe import BytePairEncoding
    from c_bpe.bpe import openai as openai_direct
except ImportError:
    pytest.skip("c_bpe package not installed. Build with: pip install -e c_bpe/",
                allow_module_level=True)


# ---------------------------------------------------------------------------
# Module structure
# ---------------------------------------------------------------------------

def test_package_metadata():
    assert hasattr(c_bpe, "__version__")
    assert isinstance(c_bpe.__version__, str)
    assert len(c_bpe.__version__.split(".")) >= 2


def test_module_structure():
    assert hasattr(c_bpe, "openai")
    assert hasattr(c_bpe, "BytePairEncoding")
    assert hasattr(c_bpe, "bpe")
    assert hasattr(openai, "cl100k_base")
    assert hasattr(openai, "o200k_base")
    assert hasattr(openai, "Tokenizer")
    assert hasattr(openai, "ParallelOptions")
    assert hasattr(openai, "get_num_threads")
    assert hasattr(openai, "is_cached_cl100k")
    assert hasattr(openai, "is_cached_o200k")


# ---------------------------------------------------------------------------
# cl100k_base tokenizer
# ---------------------------------------------------------------------------

@pytest.fixture(scope="module")
def cl100k():
    return openai.cl100k_base()


def test_encode_str(cl100k):
    tokens = cl100k.encode("Hello, world!")
    assert isinstance(tokens, list)
    assert all(isinstance(t, int) for t in tokens)
    assert len(tokens) > 0


def test_decode_roundtrip(cl100k):
    text = "Hello, world!"
    tokens = cl100k.encode(text)
    assert cl100k.decode(tokens) == text


def test_count_matches_encode_length(cl100k):
    text = "The quick brown fox jumps over the lazy dog."
    tokens = cl100k.encode(text)
    assert cl100k.count(text) == len(tokens)


def test_count_till_limit_under_budget(cl100k):
    text = "Hello, world!"
    n = cl100k.count(text)
    # limit >= count → returns count
    assert cl100k.count_till_limit(text, n) == n
    assert cl100k.count_till_limit(text, n + 5) == n


def test_count_till_limit_over_budget(cl100k):
    text = "Hello, world!"
    n = cl100k.count(text)
    if n > 1:
        assert cl100k.count_till_limit(text, n - 1) is None


def test_encode_empty_string(cl100k):
    assert cl100k.encode("") == []
    assert cl100k.count("") == 0


def test_decode_invalid_tokens_returns_none(cl100k):
    # Token ID far outside vocabulary should produce None (invalid UTF-8 bytes)
    result = cl100k.decode([999_999_999])
    assert result is None


def test_bpe_method_returns_bpe(cl100k):
    bpe = cl100k.bpe()
    assert isinstance(bpe, BytePairEncoding)


def test_bpe_count_and_encode(cl100k):
    bpe = cl100k.bpe()
    sample = b"Hello"
    n = bpe.count(sample)
    tokens = bpe.encode_via_backtracking(sample)
    assert isinstance(tokens, list)
    assert n == len(tokens)
    assert bpe.decode_tokens(tokens) == sample


def test_encode_batch(cl100k):
    texts = ["Hello, world!", "The quick brown fox.", "BPE tokenizer test."]
    result, total, elapsed = cl100k.encode_batch(texts)
    assert isinstance(result, list)
    assert len(result) == len(texts)
    for sub in result:
        assert isinstance(sub, list)
        assert all(isinstance(t, int) for t in sub)
    assert total == sum(len(s) for s in result)
    assert isinstance(elapsed, float)
    assert elapsed >= 0.0


def test_encode_batch_parallel(cl100k):
    texts = ["Hello, world!", "The quick brown fox.", "OpenMP batch test."]
    result_tuple = cl100k.encode_batch_parallel(texts)
    tokens, total, elapsed, nthreads = result_tuple
    assert isinstance(tokens, list)
    assert len(tokens) == len(texts)
    assert total >= len(texts)   # at minimum one token per string
    assert isinstance(elapsed, float)
    assert isinstance(nthreads, int)
    assert nthreads >= 1


def test_decode_batch(cl100k):
    texts = ["Hello", "world", "foo bar"]
    batch = [cl100k.encode(t) for t in texts]
    decoded = cl100k.decode_batch(batch)
    assert decoded == texts


# ---------------------------------------------------------------------------
# o200k_base tokenizer
# ---------------------------------------------------------------------------

@pytest.fixture(scope="module")
def o200k():
    return openai.o200k_base()


def test_o200k_encode_decode(o200k):
    text = "Hello, world!"
    tokens = o200k.encode(text)
    assert len(tokens) > 0
    assert o200k.decode(tokens) == text


def test_o200k_count(o200k):
    text = "GPT-4o tokenizer."
    assert o200k.count(text) == len(o200k.encode(text))


# ---------------------------------------------------------------------------
# Cache / thread helpers
# ---------------------------------------------------------------------------

def test_is_cached_after_retrieval():
    openai.cl100k_base()  # ensure cached
    assert openai.is_cached_cl100k() is True


def test_get_num_threads():
    n = openai.get_num_threads()
    assert isinstance(n, int)
    assert n >= 1


# ---------------------------------------------------------------------------
# Unicode / multibyte correctness
# ---------------------------------------------------------------------------

def test_unicode_roundtrip(cl100k):
    texts = [
        "\u00e9l\u00e8ve",       # French école  
        "\u4e2d\u6587\u6d4b\u8bd5",  # Chinese
        "\U0001f600",              # Emoji
    ]
    for t in texts:
        assert cl100k.decode(cl100k.encode(t)) == t


def test_long_text(cl100k):
    text = "Hello world! " * 500
    tokens = cl100k.encode(text)
    assert cl100k.count(text) == len(tokens)
    assert cl100k.decode(tokens) == text
