"""
Python bindings for the BPE (Byte Pair Encoding) Rust implementation.

Key Components:
    bpe: Module providing the core BPE functionality
    openai: Module providing OpenAI tokenizers
    BytePairEncoding: The core BPE implementation class

Project Dependencies:
    This file uses: rs_bpe.bpe: The Rust extension module
"""

# Package metadata
__version__ = "0.1.0"

# Define what will be exported
__all__ = ["BytePairEncoding", "bpe", "openai"]

try:
    # Import directly from bpe module when accessed
    class _BpeModuleProxy:
        def __getattr__(self, name):
            import importlib
            bpe_module = importlib.import_module("rs_bpe.bpe")
            return getattr(bpe_module, name)

    bpe = _BpeModuleProxy()
    
    # Direct re-exports of frequently used items
    from rs_bpe.bpe import BytePairEncoding, openai
except ImportError:
    # If the extension module is not available, provide useful error message
    import sys
    print("Error: Failed to import BPE extension module", file=sys.stderr)
    print("Make sure to build the extension with: maturin develop", file=sys.stderr)
