/* tokenizer.h — PCRE2-based pretokenizer + Tokenizer.
 *
 * C port of bpe-openai/src/lib.rs Tokenizer and Pretokenizer structs.
 * Uses PCRE2 (bundled in third_party/pcre2/) for Unicode-aware regex matching.
 */
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bpe.h"
#include "pretok.h"

#ifndef C_BPE_NO_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Pretokenizer (PCRE2-based)
 * ========================================================================= */

#ifndef C_BPE_NO_PCRE2

/* Maximum number of patterns in a pretokenizer */
#define PRETOK_MAX_PATTERNS 8

typedef struct Pretokenizer {
    pcre2_code   *patterns[PRETOK_MAX_PATTERNS];
    pcre2_code   *combined;   /* combined alternation for single-match path */
    bool          lookahead[PRETOK_MAX_PATTERNS]; /* strip last byte of match? */
    int           npatterns;
} Pretokenizer;

Pretokenizer *pretokenizer_new(const char **patterns, const bool *lookaheads,
                               int npatterns, int *error_code,
                               size_t *error_offset);
void          pretokenizer_free(Pretokenizer *pre);

#else
typedef struct Pretokenizer Pretokenizer; /* opaque, unused */
#endif

/* =========================================================================
 * Tokenizer
 * ========================================================================= */

typedef struct Tokenizer {
    BytePairEncoding *bpe;
    Pretokenizer     *pre;        /* PCRE2-based, may be NULL */
    pretok_fn         native_pre; /* hand-coded, may be NULL */
} Tokenizer;

typedef struct {
    uint32_t *tokens;       /* heap-allocated; caller must free */
    size_t    ntokens;
} TokenizerEncodeResult;

Tokenizer *tokenizer_new(BytePairEncoding *bpe, const char *pattern,
                         int *error_code, size_t *error_offset);

Tokenizer *tokenizer_new_lookahead(BytePairEncoding *bpe,
                                   const char **patterns,
                                   const bool  *lookaheads,
                                   int          npatterns,
                                   int *error_code, size_t *error_offset);

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
    const Pretokenizer *pre;      /* PCRE2 path (may be NULL) */
    pretok_fn           native;   /* native path (may be NULL) */
    const uint8_t      *text;
    size_t              text_len;
    int                 pat_idx;
    size_t              offset;    /* next search start */
#ifndef C_BPE_NO_PCRE2
    pcre2_match_data   *mdata;
#endif
    /* pending match from previous pattern scan */
    size_t              pending_start;
    size_t              pending_end;
    int                 has_pending;
} PretokIter;

PretokIter pretok_iter_new(const Pretokenizer *pre,
                           const uint8_t *text, size_t text_len);
bool       pretok_iter_next(PretokIter *it, size_t *out_start, size_t *out_end);
void       pretok_iter_free(PretokIter *it);

#ifdef __cplusplus
}
#endif
#endif /* TOKENIZER_H */
