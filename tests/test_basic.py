"""
Basic tests for the c_bpe package.
"""

import pytest

try:
    import c_bpe
    from c_bpe import BytePairEncoding, openai
    from c_bpe.bpe import openai as openai_direct
except ImportError:
    pytest.skip("c_bpe package not installed", allow_module_level=True)


def test_package_metadata():
    """Test basic package metadata."""
    assert hasattr(c_bpe, "__version__")
    assert isinstance(c_bpe.__version__, str)
    assert len(c_bpe.__version__.split(".")) >= 2


def test_module_structure():
    """Test that the module structure is as expected."""
    # Package level imports should work
    assert hasattr(c_bpe, "openai")
    assert hasattr(c_bpe, "BytePairEncoding")
    assert hasattr(c_bpe, "bpe")

    # The openai submodule should have the expected functions
    assert hasattr(openai, "cl100k_base")
    assert hasattr(openai, "o200k_base")
    assert hasattr(openai, "Tokenizer")

    # The direct import should match the re-exported one
    assert openai is openai_direct


def test_tokenizer_basic():
    """Test basic tokenizer functionality."""
    # Get the tokenizer
    tokenizer = openai.cl100k_base()

    # Test encoding
    tokens = tokenizer.encode("Hello, world!")
    assert isinstance(tokens, list)
    assert all(isinstance(t, int) for t in tokens)
    assert len(tokens) > 0

    # Test decoding
    text = tokenizer.decode(tokens)
    assert text == "Hello, world!"

    # Test count
    count = tokenizer.count("Hello, world!")
    assert count == len(tokens)

    # Test count_till_limit with a limit equal to token count
    # Should return the count since it fits exactly
    exact_limit = count
    exact_count = tokenizer.count_till_limit("Hello, world!", exact_limit)
    assert exact_count == count

    # Test with a higher limit
    # Should still return the count
    high_limit = count + 5
    high_count = tokenizer.count_till_limit("Hello, world!", high_limit)
    assert high_count == count

    # Test with a lower limit
    # Should return None since it exceeds the limit
    low_limit = count - 1
    low_count = tokenizer.count_till_limit("Hello, world!", low_limit)
    assert low_count is None


def test_bpe_functionality():
    """Test BPE functionality."""
    # Get the tokenizer and BPE
    tokenizer = openai.cl100k_base()
    bpe = tokenizer.bpe()

    # Verify the BPE instance
    assert isinstance(bpe, BytePairEncoding)

    # Test BPE methods
    sample = b"Hello"
    count = bpe.count(sample)
    assert count > 0

    # Test encoding
    tokens = bpe.encode_via_backtracking(sample)
    assert isinstance(tokens, list)
    assert all(isinstance(t, int) for t in tokens)

    # Test decoding
    decoded = bpe.decode_tokens(tokens)
    assert decoded == sample
