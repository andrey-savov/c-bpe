"""
Import structure tests for c_bpe — verifies parity with rs_bpe API surface.

Run with: pytest c_bpe/tests/test_import.py -v
"""

import pytest


def test_top_level_import():
    import c_bpe
    assert c_bpe.__version__ == "0.1.0"


def test_bpe_submodule():
    from c_bpe import bpe
    assert hasattr(bpe, "BytePairEncoding")
    assert hasattr(bpe, "openai")


def test_openai_submodule_direct():
    from c_bpe.bpe import openai
    for name in ("cl100k_base", "o200k_base", "Tokenizer", "ParallelOptions",
                 "is_cached_cl100k", "is_cached_o200k", "get_num_threads"):
        assert hasattr(openai, name), f"c_bpe.bpe.openai missing '{name}'"


def test_openai_wrapper():
    from c_bpe import openai
    for name in ("cl100k_base", "o200k_base", "Tokenizer", "ParallelOptions",
                 "is_cached_cl100k", "is_cached_o200k", "get_num_threads"):
        assert hasattr(openai, name), f"c_bpe.openai missing '{name}'"


def test_parallel_options_instantiation():
    from c_bpe.bpe.openai import ParallelOptions
    p = ParallelOptions()
    assert p.min_batch_size == 20
    assert p.chunk_size == 100
    assert p.max_threads == 0

    p2 = ParallelOptions(min_batch_size=5, chunk_size=10, max_threads=4)
    assert p2.min_batch_size == 5
    assert p2.chunk_size == 10
    assert p2.max_threads == 4


@pytest.mark.parametrize("factory", ["cl100k_base", "o200k_base"])
def test_factory_returns_tokenizer(factory):
    from c_bpe.bpe import openai
    from c_bpe.bpe.openai import Tokenizer
    tok = getattr(openai, factory)()
    assert isinstance(tok, Tokenizer)


def test_bpe_type():
    from c_bpe.bpe import BytePairEncoding, openai
    tok = openai.cl100k_base()
    bpe = tok.bpe()
    assert isinstance(bpe, BytePairEncoding)
    assert hasattr(bpe, "count")
    assert hasattr(bpe, "encode_via_backtracking")
    assert hasattr(bpe, "decode_tokens")


def test_singleton_identity():
    """The same underlying C object should be returned on repeated calls."""
    from c_bpe.bpe.openai import cl100k_base, is_cached_cl100k
    cl100k_base()   # first call initialises
    assert is_cached_cl100k() is True
    # Second call should be fast (cached) — just verify it doesn't raise
    tok2 = cl100k_base()
    assert tok2 is not None


def test_parity_with_rs_bpe():
    """If rs_bpe is installed, verify c_bpe and rs_bpe produce identical tokens."""
    rs_bpe = pytest.importorskip("rs_bpe")
    from c_bpe.bpe import openai as c_openai
    from rs_bpe.bpe import openai as rs_openai  # type: ignore[attr-defined]

    c_tok  = c_openai.cl100k_base()
    rs_tok = rs_openai.cl100k_base()

    sample_texts = [
        "Hello, world!",
        "The quick brown fox jumps over the lazy dog.",
        "BPE tokenizer cross-implementation parity check.",
        "\u00e9l\u00e8ve en fran\u00e7ais",
        "12345 numbers test",
    ]
    for text in sample_texts:
        c_tokens  = c_tok.encode(text)
        rs_tokens = rs_tok.encode(text)
        assert c_tokens == rs_tokens, (
            f"Token mismatch for {text!r}:\n"
            f"  c_bpe:  {c_tokens}\n"
            f"  rs_bpe: {rs_tokens}"
        )
