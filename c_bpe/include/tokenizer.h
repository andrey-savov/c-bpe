/* tokenizer.h — Tokenizer with native pretokenizer.
 *
 * C port of bpe-openai/src/lib.rs Tokenizer struct.
 * Uses hand-coded pretokenizer functions (pretok_cl100k / pretok_o200k)
 * with PCRE2 UCD tables for Unicode property classification.
 */
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bpe.h"
#include "pretok.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Tokenizer
 * ========================================================================= */

typedef struct Tokenizer {
    BytePairEncoding *bpe;
    pretok_fn         native_pre; /* hand-coded pretokenizer, may be NULL */
} Tokenizer;

typedef struct {
    uint32_t *tokens;       /* heap-allocated; caller must free */
    size_t    ntokens;
} TokenizerEncodeResult;

/** Create a tokenizer with a hand-coded (native) pretokenizer function. */
Tokenizer *tokenizer_new_native(BytePairEncoding *bpe, pretok_fn fn);

void tokenizer_free(Tokenizer *tok);

/**
 * Encode UTF-8 text with pretokenization.
 * Returns heap-allocated array of token ids; length in *out_n.
 */
uint32_t *tokenizer_encode(const Tokenizer *tok,
                           const uint8_t *text, size_t text_len,
                           size_t *out_n);

size_t    tokenizer_count(const Tokenizer *tok,
                          const uint8_t *text, size_t text_len);

/** Returns SIZE_MAX if limit exceeded. */
size_t    tokenizer_count_till_limit(const Tokenizer *tok,
                                     const uint8_t *text, size_t text_len,
                                     size_t token_limit);

/* =========================================================================
 * Pretokenizer iteration (for external use, e.g., from Python)
 * ========================================================================= */

typedef struct PretokIter {
    pretok_fn           native;   /* native pretok function (may be NULL) */
    const uint8_t      *text;
    size_t              text_len;
    size_t              offset;    /* next search start */
} PretokIter;

PretokIter pretok_iter_new_native_ext(pretok_fn fn,
                                      const uint8_t *text, size_t text_len);
bool       pretok_iter_next(PretokIter *it, size_t *out_start, size_t *out_end);
void       pretok_iter_free(PretokIter *it);

#ifdef __cplusplus
}
#endif
#endif /* TOKENIZER_H */
