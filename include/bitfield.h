/* bitfield.h — simple bit array with predecessor/successor queries.
 *
 * Ported from bpe/src/bitfield.rs.
 * All bits initialised to 1. Two set bits are at most 128 bits apart
 * in normal BPE usage, so a simple scan is O(1) in practice.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- Portable count-trailing/leading zeros --------------------------------
 * MSVC does not have __builtin_ctzll/__builtin_clzll; use intrinsics instead.
 * -------------------------------------------------------------------------- */
#ifdef _MSC_VER
#  include <intrin.h>
static __inline int _cbpe_ctzll(unsigned __int64 v) {
    unsigned long idx = 0;
    _BitScanForward64(&idx, v);
    return (int)idx;
}
static __inline int _cbpe_clzll(unsigned __int64 v) {
    unsigned long idx = 0;
    _BitScanReverse64(&idx, v);
    return 63 - (int)idx;
}
#  define __builtin_ctzll _cbpe_ctzll
#  define __builtin_clzll _cbpe_clzll
#endif

typedef struct {
    uint64_t *words;
    size_t    nwords;
} BitField;

/* Allocate a bitfield with `bits` bits, all set to 1. */
static inline BitField bitfield_new(size_t bits) {
    size_t nwords = (bits + 63) / 64;
    BitField bf;
    bf.nwords = nwords;
    bf.words  = (uint64_t *)malloc(nwords * sizeof(uint64_t));
    for (size_t i = 0; i < nwords; i++) bf.words[i] = UINT64_MAX;
    return bf;
}

static inline void bitfield_free(BitField *bf) {
    free(bf->words);
    bf->words  = NULL;
    bf->nwords = 0;
}

static inline int bitfield_is_set(const BitField *bf, size_t bit) {
    return (bf->words[bit / 64] >> (bit % 64)) & 1;
}

static inline void bitfield_clear(BitField *bf, size_t bit) {
    bf->words[bit / 64] &= ~((uint64_t)1 << (bit % 64));
}

/* Return the index of the next set bit >= bit. */
static inline size_t bitfield_successor(const BitField *bf, size_t bit) {
    size_t word_idx = bit / 64;
    int    bit_idx  = (int)(bit % 64);
    uint64_t word   = bf->words[word_idx] >> bit_idx;
    if (word) {
        return bit + (size_t)__builtin_ctzll(word);
    }
    for (;;) {
        word_idx++;
        word = bf->words[word_idx];
        if (word) return word_idx * 64 + (size_t)__builtin_ctzll(word);
    }
}

/* Return the index of the previous set bit <= bit. */
static inline size_t bitfield_predecessor(const BitField *bf, size_t bit) {
    size_t word_idx = bit / 64;
    int    bit_idx  = (int)(bit % 64);
    uint64_t word   = bf->words[word_idx] << (63 - bit_idx);
    if (word) {
        return bit - (size_t)__builtin_clzll(word);
    }
    for (;;) {
        word_idx--;
        word = bf->words[word_idx];
        if (word) return word_idx * 64 + 63 - (size_t)__builtin_clzll(word);
    }
}

/* In-place initialisation (alias matching bpe_core.c call convention) */
static inline void bitfield_init(BitField *bf, size_t bits) {
    *bf = bitfield_new(bits);
}

/* Ensure capacity for `bits` bits, growing if needed; reset all to 1.
 * Avoids malloc/free when the existing buffer is already large enough. */
static inline void bitfield_reset(BitField *bf, size_t bits) {
    size_t need = (bits + 63) / 64;
    if (need > bf->nwords) {
        bf->nwords = need;
        bf->words  = (uint64_t *)realloc(bf->words, need * sizeof(uint64_t));
    }
    memset(bf->words, 0xFF, need * sizeof(uint64_t));
}
