/* gen_precomputed.c — Standalone program that builds the BPE data structures
 * and serializes them as a binary blob to stdout.
 *
 * Built and run during codegen (by gen_precomputed.py) to produce
 * precomputed_cl100k.bin.gz and precomputed_o200k.bin.gz.
 * At build time these are decompressed to .bin and embedded via C23 #embed.
 *
 * Usage:  gen_precomputed cl100k | gen_precomputed o200k
 * Writes binary data to stdout.
 *
 * Binary format (all little-endian, no padding):
 *   Header:
 *     uint32_t num_tokens
 *     uint64_t hash_factor
 *     uint32_t bytesmap_capacity
 *     uint32_t bytesmap_count
 *     uint32_t pairmap_capacity
 *     uint32_t pairmap_count
 *     -- for each of 3 automatons (longest, overlapping, overlapping_rev):
 *        int32_t  da_size
 *        int32_t  noutputs
 *        int32_t  kind
 *   Arrays (in order):
 *     split_left      [num_tokens × uint32_t]
 *     split_right     [num_tokens × uint32_t]
 *     next_prefix_match [num_tokens × uint32_t]
 *     bytesmap slots  [bytesmap_capacity × BytesMapSlot(8 bytes)]
 *     pairmap slots   [pairmap_capacity × PairMapSlot(12 bytes)]
 *     -- for each of 3 automatons:
 *        cells         [da_size × AcCell(16 bytes)]
 *        outputs       [noutputs × AcOutput(12 bytes)]
 *        da_base       [da_size × int32_t]
 *        da_check      [da_size × int32_t]
 */

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "bpe.h"
#include "fnv_hash.h"
#include "ac_bpe.h"

/* The dict headers are included from src/ */
#include "dict_cl100k.h"
#include "dict_o200k.h"

static void write_bytes(const void *data, size_t n) {
    if (fwrite(data, 1, n, stdout) != n) {
        fprintf(stderr, "write error\n");
        exit(1);
    }
}

static void serialize_automaton(const AcAutomaton *a) {
    /* cells */
    write_bytes(a->cells, (size_t)a->da_size * sizeof(AcCell));
    /* outputs */
    write_bytes(a->outputs, (size_t)a->noutputs * sizeof(AcOutput));
    /* da_base */
    write_bytes(a->da_base, (size_t)a->da_size * sizeof(int32_t));
    /* da_check */
    write_bytes(a->da_check, (size_t)a->da_size * sizeof(int32_t));
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "cl100k") != 0 &&
                      strcmp(argv[1], "o200k") != 0)) {
        fprintf(stderr, "Usage: %s cl100k|o200k\n", argv[0]);
        return 1;
    }

#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    const uint8_t  *all_tok;
    const uint32_t *tok_starts;
    uint32_t        num_tok;
    uint64_t        hash_factor;

    if (strcmp(argv[1], "cl100k") == 0) {
        all_tok     = CL100K_ALL_TOKENS;
        tok_starts  = CL100K_TOKEN_STARTS;
        num_tok     = CL100K_NUM_TOKENS;
        hash_factor = CL100K_HASH_FACTOR;
    } else {
        all_tok     = O200K_ALL_TOKENS;
        tok_starts  = O200K_TOKEN_STARTS;
        num_tok     = O200K_NUM_TOKENS;
        hash_factor = O200K_HASH_FACTOR;
    }

    fprintf(stderr, "Building BPE for %s (%u tokens) ...\n", argv[1], num_tok);
    BytePairEncoding *bpe = bpe_from_dictionary(all_tok, tok_starts,
                                                num_tok, hash_factor);
    fprintf(stderr, "BPE built. Serializing ...\n");

    /* ---- Header ---- */
    write_bytes(&bpe->num_tokens, sizeof(uint32_t));
    write_bytes(&bpe->hash_factor, sizeof(uint64_t));

    write_bytes(&bpe->hash_to_token.capacity, sizeof(uint32_t));
    write_bytes(&bpe->hash_to_token.count, sizeof(uint32_t));

    write_bytes(&bpe->pair_lookup.capacity, sizeof(uint32_t));
    write_bytes(&bpe->pair_lookup.count, sizeof(uint32_t));

    /* 3 automaton headers */
    const AcAutomaton *autos[3] = {
        bpe->longest_searcher,
        bpe->overlapping_searcher,
        bpe->overlapping_searcher_rev,
    };
    for (int i = 0; i < 3; i++) {
        write_bytes(&autos[i]->da_size, sizeof(int32_t));
        write_bytes(&autos[i]->noutputs, sizeof(int32_t));
        int32_t kind = (int32_t)autos[i]->kind;
        write_bytes(&kind, sizeof(int32_t));
    }

    /* ---- Arrays ---- */
    write_bytes(bpe->split_left, num_tok * sizeof(uint32_t));
    write_bytes(bpe->split_right, num_tok * sizeof(uint32_t));
    write_bytes(bpe->next_prefix_match, num_tok * sizeof(uint32_t));

    /* hash_to_token slots */
    write_bytes(bpe->hash_to_token.slots,
                bpe->hash_to_token.capacity * sizeof(BytesMapSlot));

    /* pair_lookup slots */
    write_bytes(bpe->pair_lookup.slots,
                bpe->pair_lookup.capacity * sizeof(PairMapSlot));

    /* 3 automatons */
    for (int i = 0; i < 3; i++) {
        serialize_automaton(autos[i]);
    }

    fprintf(stderr, "Done. Cleaning up.\n");
    bpe_free(bpe);
    return 0;
}
