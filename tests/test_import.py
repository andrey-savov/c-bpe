#!/usr/bin/env python3

print("=== Testing Import Patterns ===")

# Test 1: Import the bpe module directly
try:
    print("\nTest 1: from c_bpe import bpe")
    from c_bpe import bpe
    print(f"OK Success! bpe module imported: {bpe}")
    print(f"bpe module attributes: {dir(bpe)}")
except ImportError as e:
    print(f"FAIL Failed: {e}")

# Test 2: Import from bpe.openai
try:
    print("\nTest 2: from c_bpe.bpe import openai")
    from c_bpe.bpe import openai
    print(f"OK Success! openai module imported: {openai}")
    print(f"openai attributes: {dir(openai)}")
except ImportError as e:
    print(f"FAIL Failed: {e}")

# Test 3: Import openai directly from package
try:
    print("\nTest 3: from c_bpe import openai")
    from c_bpe import openai
    print(f"OK Success! openai module imported: {openai}")
    print(f"openai attributes: {dir(openai)}")
except ImportError as e:
    print(f"FAIL Failed: {e}")

# Test 4: Import BytePairEncoding
try:
    print("\nTest 4: from c_bpe import BytePairEncoding")
    from c_bpe import BytePairEncoding
    print(f"OK Success! BytePairEncoding imported: {BytePairEncoding}")
except ImportError as e:
    print(f"FAIL Failed: {e}")

# Test 5: Full package usage
try:
    print("\nTest 5: Full package usage")
    import c_bpe
    tokenizer = c_bpe.openai.cl100k_base()
    tokens = tokenizer.encode("Hello, world!")
    bpe_obj = tokenizer.bpe()

    print("OK Success! Full package usage works")
    print(f"  - Tokens: {tokens}")
    print(f"  - BPE object: {bpe_obj}")
    print(f"  - c_bpe.__all__: {c_bpe.__all__}")  # type: ignore
except (ImportError, AttributeError) as e:
    print(f"FAIL Failed: {e}")

print("\n=== Test Complete ===")
