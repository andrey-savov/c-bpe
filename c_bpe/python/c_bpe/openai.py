"""
OpenAI tokenizer interface for the C BPE implementation.

Key Components:
    cl100k_base(): Returns the cl100k tokenizer (GPT-3.5/4)
    o200k_base():  Returns the o200k tokenizer (GPT-4o)
    Tokenizer:     Tokenizer class with encode/decode methods
    is_cached_cl100k(): Check if cl100k is already initialised
    is_cached_o200k():  Check if o200k is already initialised
    get_num_threads():  Number of OpenMP threads available
"""

__all__ = [
    "Tokenizer",
    "ParallelOptions",
    "cl100k_base",
    "o200k_base",
    "is_cached_cl100k",
    "is_cached_o200k",
    "get_num_threads",
]

try:
    from c_bpe.bpe.openai import (
        Tokenizer,
        ParallelOptions,
        cl100k_base,
        o200k_base,
        is_cached_cl100k,
        is_cached_o200k,
        get_num_threads,
    )
except ImportError:
    import sys
    print("Error: Failed to import c_bpe.bpe.openai.", file=sys.stderr)
    print("Build with: pip install -e c_bpe/", file=sys.stderr)
