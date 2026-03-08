"""
Python bindings for the BPE C implementation.

Key Components:
    bpe: Module providing the core BPE functionality
    openai: Module providing OpenAI tokenizers
    BytePairEncoding: The core BPE implementation class
"""

__version__ = "0.1.0"

__all__ = ["BytePairEncoding", "bpe", "openai"]

try:
    from c_bpe.bpe import BytePairEncoding, openai
    import c_bpe.bpe as bpe
except ImportError:
    import sys
    print("Error: Failed to import c_bpe extension module.", file=sys.stderr)
    print("Make sure to build with: pip install -e .", file=sys.stderr)
