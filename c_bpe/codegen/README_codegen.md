# Codegen: Precomputed BPE Data

The `c_bpe` extension ships with precomputed binary blobs that contain all
BPE data structures (AC automata, hash maps, split/prefix tables) for each
tokenizer model. This eliminates the ~27 s cold-start that would otherwise
be needed to build these structures from scratch at runtime.

## Files

| File | Purpose |
|------|---------|
| `data/precomputed_cl100k.bin.gz` | Gzipped binary blob for cl100k_base |
| `data/precomputed_o200k.bin.gz`  | Gzipped binary blob for o200k_base  |
| `codegen/gen_precomputed.py`     | Two-mode script (generate / emit)   |
| `codegen/gen_precomputed.c`      | C serializer (used by `generate`)   |

## How it works

### Build time (automatic)

`setup.py` calls `gen_precomputed.py emit` which:

1. Reads the committed `.bin.gz` blobs from `data/`
2. Decompresses with Python's built-in `gzip` module
3. Writes raw `precomputed_cl100k.bin` and `precomputed_o200k.bin` into `src/`
4. The extension compiles with `-DBPE_USE_PRECOMPUTED=1` and uses C23 `#embed`
   to include the binary data directly

No C compiler is needed for this step (beyond the one building the extension
itself).  Requires C23 support (`/std:clatest` on MSVC, `-std=c23` on GCC/Clang).

### Regenerating the blobs (manual, developer-only)

If you change the dictionary data, hash functions, or AC automaton
construction, you must regenerate the blobs:

```bash
# Linux / macOS
cd c_bpe
python codegen/gen_precomputed.py generate \
    --src-dir src --data-dir data

# Windows (from a VS Developer Command Prompt, or vcvarsall x64)
cd c_bpe
python codegen\gen_precomputed.py generate ^
    --src-dir src --data-dir data
```

This will:

1. Compile `codegen/gen_precomputed.c` (links `ac_bpe.c` + `bpe_core.c`)
2. Run the resulting binary once per model (cl100k, o200k)
3. Write the output as `data/precomputed_*.bin.gz` (gzip -9)

Each run takes ~30 s per model (dominated by AC automaton construction).

After regenerating, commit the updated `.bin.gz` files.

### Compiler requirements for `generate`

| Platform | Compiler | Notes |
|----------|----------|-------|
| Windows  | MSVC (cl.exe) | Needs `/std:c11 /experimental:c11atomics`. Run from a VS Developer Command Prompt or after `vcvarsall.bat x64`. |
| Linux    | gcc or clang  | `-std=c11` is used automatically. |
| macOS    | clang (Xcode) | Works out of the box. |

## Binary format

The `.bin` blob (before gzip) is a flat little-endian binary:

```
Header:
  uint32_t  num_tokens
  uint64_t  hash_factor
  uint32_t  bytesmap_capacity, bytesmap_count
  uint32_t  pairmap_capacity,  pairmap_count
  3 × { int32_t da_size, noutputs, kind }     (automaton headers)

Arrays:
  split_left        [num_tokens × uint32_t]
  split_right       [num_tokens × uint32_t]
  next_prefix_match [num_tokens × uint32_t]
  bytesmap_slots    [bytesmap_capacity × 8 bytes]
  pairmap_slots     [pairmap_capacity × 12 bytes]
  3 × {
    cells    [da_size × 16 bytes]   (AcCell)
    outputs  [noutputs × 12 bytes]  (AcOutput)
    da_base  [da_size × 4 bytes]
    da_check [da_size × 4 bytes]
  }
```
