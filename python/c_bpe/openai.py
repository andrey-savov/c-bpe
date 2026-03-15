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

import gzip
from pathlib import Path

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
        cl100k_base as _cl100k_base_c,
        o200k_base as _o200k_base_c,
        is_cached_cl100k,
        is_cached_o200k,
        get_num_threads,
    )
except ImportError:
    import sys
    print("Error: Failed to import c_bpe.bpe.openai.", file=sys.stderr)
    print("Build with: pip install -e c_bpe/", file=sys.stderr)

_PKG_DIR = Path(__file__).parent


def _ensure_decompressed(name: str) -> None:
    """Decompress precomputed_<name>.bin.gz to .bin if the .bin is missing."""
    bin_file = _PKG_DIR / f"precomputed_{name}.bin"
    if bin_file.exists():
        return
    gz_file = _PKG_DIR / f"precomputed_{name}.bin.gz"
    if gz_file.exists():
        with gzip.open(gz_file, "rb") as f_in:
            data = f_in.read()
        with open(bin_file, "wb") as f_out:
            f_out.write(data)


def cl100k_base():
    _ensure_decompressed("cl100k")
    return _cl100k_base_c()


def o200k_base():
    _ensure_decompressed("o200k")
    return _o200k_base_c()
