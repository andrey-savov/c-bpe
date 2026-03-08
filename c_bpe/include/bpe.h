/* bpe.h — BytePairEncoding struct and API.
 *
 * C port of bpe/src/byte_pair_encoding.rs.
 */
#ifndef BPE_H
#define BPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fnv_hash.h"
#include "ac_bpe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * BytePairEncoding
 * ========================================================================= */

typedef struct BytePairEncoding {
    /* Concatenated bytes of all tokens */
    uint8_t  *all_tokens;
    /* token_starts[i]..token_starts[i+1] is the byte range for token i */
    uint32_t *token_starts;  /* length = num_tokens + 1 */
    uint32_t  num_tokens;

    /* Hash map: bpe_hash_bytes(token) -> token_id */
    BytesMap  hash_to_token;
    uint64_t  hash_factor;

    /* Split table: split_left[i], split_right[i] = the two sub-tokens that
     * were merged to form token i.  If token i is primitive,
     * split_left[i] == split_right[i] == i. */
    uint32_t *split_left;
    uint32_t *split_right;

    /* Pair lookup: (token1, token2) -> merged token */
    PairMap   pair_lookup;

    /* AC automatons */
    AcAutomaton *longest_searcher;        /* leftmost-longest */
    AcAutomaton *overlapping_searcher;    /* overlapping forward */
    AcAutomaton *overlapping_searcher_rev;/* overlapping reverse */

    /* next_prefix_match[i] = token_id of next longest proper-prefix of token i,
     * or UINT32_MAX if none. */
    uint32_t *next_prefix_match;
} BytePairEncoding;

/* =========================================================================
 * Construction / destruction
 * ========================================================================= */

/**
 * Build a BytePairEncoding from a flat token array.
 *
 * @param all_tokens    concatenated token bytes (NOT null-terminated)
 * @param token_starts  token_starts[i] = start offset of token i in all_tokens;
 *                      must have num_tokens+1 entries (last = total bytes)
 * @param num_tokens    number of tokens
 * @param hash_factor   collision-free hash multiplier (must be != 0);
 *                      use 1 if unsure then validate with bpe_self_check().
 * @return heap-allocated BytePairEncoding*, caller must call bpe_free().
 */
BytePairEncoding *bpe_from_dictionary(const uint8_t *all_tokens,
                                      const uint32_t *token_starts,
                                      uint32_t        num_tokens,
                                      uint64_t        hash_factor);

void bpe_free(BytePairEncoding *bpe);

/* =========================================================================
 * Basic accessors
 * ========================================================================= */

static inline uint32_t bpe_num_tokens(const BytePairEncoding *bpe) {
    return bpe->num_tokens;
}

static inline uint32_t bpe_token_len(const BytePairEncoding *bpe, uint32_t id) {
    return bpe->token_starts[id + 1] - bpe->token_starts[id];
}

static inline const uint8_t *bpe_token_bytes(const BytePairEncoding *bpe,
                                              uint32_t id, uint32_t *len_out) {
    uint32_t s = bpe->token_starts[id];
    uint32_t e = bpe->token_starts[id + 1];
    if (len_out) *len_out = e - s;
    return bpe->all_tokens + s;
}

/* =========================================================================
 * Validity / lookup helpers
 * ========================================================================= */

bool bpe_is_valid_token_pair(const BytePairEncoding *bpe,
                             uint32_t token1, uint32_t token2);

/* Returns token id or UINT32_MAX if not found */
uint32_t bpe_find_token_by_bytes(const BytePairEncoding *bpe,
                                 const uint8_t *bytes, uint32_t len);

/* Returns the leftmost-longest match starting in text[0..], or UINT32_MAX */
uint32_t bpe_next_match(const BytePairEncoding *bpe,
                        const uint8_t *text, size_t len);

/* Returns next shorter prefix token, or UINT32_MAX */
static inline uint32_t bpe_next_prefix(const BytePairEncoding *bpe,
                                       uint32_t token_id) {
    return bpe->next_prefix_match[token_id];
}

/* =========================================================================
 * Encoding algorithms
 * ========================================================================= */

/**
 * Decode a list of token ids to bytes.
 * Caller must free the returned buffer.
 */
uint8_t *bpe_decode_tokens(const BytePairEncoding *bpe,
                           const uint32_t *tokens, size_t ntokens,
                           size_t *out_len);

/**
 * Compute for every position in text the last token ending there.
 * Returns a heap-allocated array of length text_len; caller must free it.
 */
uint32_t *bpe_encode_all_prefixes(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len);

/** Count tokens (does not produce the token list). */
size_t bpe_count(const BytePairEncoding *bpe,
                 const uint8_t *text, size_t text_len);

/** Count tokens but return SIZE_MAX if count exceeds token_limit. */
size_t bpe_count_till_limit(const BytePairEncoding *bpe,
                             const uint8_t *text, size_t text_len,
                             size_t token_limit);

/**
 * Encode via backtracking (matches tiktoken exactly).
 * Returns heap-allocated token array, length stored in *out_n.
 */
uint32_t *bpe_encode_via_backtracking(const BytePairEncoding *bpe,
                                      const uint8_t *text, size_t text_len,
                                      size_t *out_n);

/* =========================================================================
 * Scratch-buffer API  (amortise allocations across many encode/count calls)
 * ========================================================================= */

/** Opaque scratch buffer for reuse across multiple encode/count calls. */
typedef struct BpeEncScratch BpeEncScratch;

/** Allocate a new (empty) scratch buffer. */
BpeEncScratch *bpe_scratch_new(void);

/** Free a scratch buffer. */
void bpe_scratch_free(BpeEncScratch *s);

/**
 * Encode a piece using scratch buffers (no per-call malloc/free).
 * Returns a pointer into the internal scratch token array; the pointer is
 * valid until the next call on the same scratch.  *out_n receives the count.
 */
const uint32_t *bpe_encode_piece(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len,
                                  BpeEncScratch *s, size_t *out_n);

/**
 * Count tokens for a piece using scratch buffers (no per-call malloc/free).
 */
size_t bpe_count_piece(const BytePairEncoding *bpe,
                        const uint8_t *text, size_t text_len,
                        BpeEncScratch *s);

/**
 * Encode via bitfield (priority-heap BPE, also exact).
 * Returns heap-allocated token array, length stored in *out_n.
 */
uint32_t *bpe_encode_via_bitfield(const BytePairEncoding *bpe,
                                  const uint8_t *text, size_t text_len,
                                  size_t *out_n);

/**
 * Greedy leftmost-longest encoding (fast but not tiktoken-compatible).
 * Returns heap-allocated token array, length stored in *out_n.
 */
uint32_t *bpe_encode_greedy(const BytePairEncoding *bpe,
                             const uint8_t *text, size_t text_len,
                             size_t *out_n);

/**
 * Encode via prefix table (exact, often fastest for full texts).
 * Returns heap-allocated token array, length stored in *out_n.
 */
uint32_t *bpe_encode_via_table(const BytePairEncoding *bpe,
                               const uint8_t *text, size_t text_len,
                               size_t *out_n);

/**
 * Encode with minimal token count (not tiktoken-compatible).
 * Returns heap-allocated token array, length stored in *out_n.
 */
uint32_t *bpe_encode_minimal(const BytePairEncoding *bpe,
                              const uint8_t *text, size_t text_len,
                              size_t *out_n);

/* =========================================================================
 * Validation (call once after construction to verify correctness)
 * ========================================================================= */
bool bpe_self_check(const BytePairEncoding *bpe);

#ifdef __cplusplus
}
#endif

#endif /* BPE_H */
