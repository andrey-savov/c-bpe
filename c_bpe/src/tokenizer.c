/* tokenizer.c — PCRE2-based pretokenizer + Tokenizer.
 *
 * Replicates bpe-openai/src/lib.rs Tokenizer:
 *   - split text using alternating PCRE2 patterns (with optional lookahead)
 *   - encode each piece with BytePairEncoding
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include "tokenizer.h"

/* =========================================================================
 * Build helpers
 * ========================================================================= */

static pcre2_code *compile_pat(const char *pat, int *err_code,
                                size_t *err_offset) {
    uint32_t options = PCRE2_UTF | PCRE2_UCP;
    PCRE2_SIZE erroff = 0;
    int        errcode = 0;
    pcre2_code *re = pcre2_compile((PCRE2_SPTR)pat, PCRE2_ZERO_TERMINATED,
                                   options, &errcode, &erroff, NULL);
    if (!re) {
        if (err_code)   *err_code   = errcode;
        if (err_offset) *err_offset = (size_t)erroff;
    } else {
        /* JIT compile for faster matching */
        pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    }
    return re;
}

/* =========================================================================
 * Pretokenizer
 * ========================================================================= */

Pretokenizer *pretokenizer_new(const char **patterns, const bool *lookaheads,
                               int npatterns, int *error_code,
                               size_t *error_offset) {
    if (npatterns > PRETOK_MAX_PATTERNS) return NULL;
    Pretokenizer *pre = (Pretokenizer *)calloc(1, sizeof(Pretokenizer));
    for (int i = 0; i < npatterns; i++) {
        pre->patterns[i] = compile_pat(patterns[i], error_code, error_offset);
        if (!pre->patterns[i]) {
            /* Free already-compiled patterns */
            for (int j = 0; j < i; j++) pcre2_code_free(pre->patterns[j]);
            free(pre);
            return NULL;
        }
        pre->lookahead[i] = lookaheads ? lookaheads[i] : false;
    }
    pre->npatterns = npatterns;

    /* Build a combined pattern: (pat0)|(pat1)|(pat2)... */
    /* Each pattern becomes a capturing group so we can identify which matched */
    size_t total_len = 0;
    for (int i = 0; i < npatterns; i++)
        total_len += strlen(patterns[i]) + 4; /* ()|  */
    char *combined = (char *)malloc(total_len + 1);
    char *p = combined;
    for (int i = 0; i < npatterns; i++) {
        if (i > 0) *p++ = '|';
        *p++ = '(';
        size_t len = strlen(patterns[i]);
        memcpy(p, patterns[i], len);
        p += len;
        *p++ = ')';
    }
    *p = '\0';
    pre->combined = compile_pat(combined, error_code, error_offset);
    free(combined);
    /* combined is optional — fall back to per-pattern if it fails */

    return pre;
}

void pretokenizer_free(Pretokenizer *pre) {
    if (!pre) return;
    for (int i = 0; i < pre->npatterns; i++)
        if (pre->patterns[i]) pcre2_code_free(pre->patterns[i]);
    if (pre->combined) pcre2_code_free(pre->combined);
    free(pre);
}

/* =========================================================================
 * Tokenizer
 * ========================================================================= */

Tokenizer *tokenizer_new(BytePairEncoding *bpe, const char *pattern,
                         int *error_code, size_t *error_offset) {
    if (!pattern) {
        Tokenizer *tok = (Tokenizer *)calloc(1, sizeof(Tokenizer));
        tok->bpe = bpe;
        return tok;
    }
    bool la = false;
    Pretokenizer *pre = pretokenizer_new(&pattern, &la, 1,
                                         error_code, error_offset);
    if (!pre) return NULL;
    Tokenizer *tok = (Tokenizer *)calloc(1, sizeof(Tokenizer));
    tok->bpe = bpe;
    tok->pre = pre;
    return tok;
}

Tokenizer *tokenizer_new_lookahead(BytePairEncoding *bpe,
                                   const char **patterns,
                                   const bool  *lookaheads,
                                   int          npatterns,
                                   int *error_code, size_t *error_offset) {
    Pretokenizer *pre = pretokenizer_new(patterns, lookaheads, npatterns,
                                         error_code, error_offset);
    if (!pre) return NULL;
    Tokenizer *tok = (Tokenizer *)calloc(1, sizeof(Tokenizer));
    tok->bpe = bpe;
    tok->pre = pre;
    return tok;
}

void tokenizer_free(Tokenizer *tok) {
    if (!tok) return;
    pretokenizer_free(tok->pre);
    free(tok);
}

/* =========================================================================
 * Pretokeniser iteration
 * =========================================================================
 * The Rust code uses a "lookahead" trick: for patterns flagged as lookahead
 * the match's last byte belongs to the *next* piece, not the current one.
 *
 * Strategy: we run all patterns as alternatives in a priority order.
 * At each position, we try each pattern in order and take the one with the
 * smallest start position (leftmost match wins).  Within the same start,
 * take the longest.  If a lookahead match is chosen, its end is reduced by 1.
 *
 * We iterate from left to right, advancing `offset` after each match.
 * ======================================================================== */

PretokIter pretok_iter_new(const Pretokenizer *pre,
                           const uint8_t *text, size_t text_len) {
    PretokIter it;
    memset(&it, 0, sizeof(it));
    it.pre      = pre;
    it.text     = text;
    it.text_len = text_len;
    it.offset   = 0;
    it.mdata    = pcre2_match_data_create(32, NULL);
    return it;
}

bool pretok_iter_next(PretokIter *it, size_t *out_start, size_t *out_end) {
    const Pretokenizer *pre = it->pre;
    if (!pre || it->offset >= it->text_len) return false;

    /* Advance subject pointer so PCRE2 works on a shrinking buffer.
     * This avoids O(n²) behavior from patterns like \s+$ that inspect the
     * full subject.  Our patterns have no lookbehind, so this is safe. */
    const PCRE2_SPTR subj     = (PCRE2_SPTR)(it->text + it->offset);
    const PCRE2_SIZE subj_len = (PCRE2_SIZE)(it->text_len - it->offset);

    size_t match_start, match_end; /* relative to subj */
    bool   match_la = false;

    if (pre->combined) {
        /* Fast path: single match with combined alternation */
        int rc = pcre2_jit_match(pre->combined,
                                 subj, subj_len, 0,
                                 PCRE2_NOTEMPTY | PCRE2_ANCHORED,
                                 it->mdata, NULL);
        if (rc < 0) {
            /* JIT may not support this pattern — fall back to interpreter */
            if (rc == PCRE2_ERROR_JIT_BADOPTION)
                rc = pcre2_match(pre->combined,
                                 subj, subj_len, 0,
                                 PCRE2_NOTEMPTY | PCRE2_ANCHORED,
                                 it->mdata, NULL);
            if (rc < 0) return false;
        }

        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(it->mdata);
        match_start = (size_t)ov[0];
        match_end   = (size_t)ov[1];

        /* Identify which capturing group matched to determine lookahead */
        for (int i = 0; i < pre->npatterns; i++) {
            if (ov[2*(i+1)] != PCRE2_UNSET) {
                match_la = pre->lookahead[i];
                break;
            }
        }
    } else {
        /* Fallback: try each pattern */
        match_start = subj_len;
        match_end   = subj_len;

        for (int i = 0; i < pre->npatterns; i++) {
            int rc = pcre2_jit_match(pre->patterns[i],
                                     subj, subj_len, 0,
                                     PCRE2_NOTEMPTY | PCRE2_ANCHORED,
                                     it->mdata, NULL);
            if (rc == PCRE2_ERROR_JIT_BADOPTION)
                rc = pcre2_match(pre->patterns[i],
                                 subj, subj_len, 0,
                                 PCRE2_NOTEMPTY | PCRE2_ANCHORED,
                                 it->mdata, NULL);
            if (rc < 0) continue;

            PCRE2_SIZE *ov = pcre2_get_ovector_pointer(it->mdata);
            size_t ms = (size_t)ov[0];
            size_t me = (size_t)ov[1];

            if (ms < match_start || (ms == match_start && me > match_end)) {
                match_start = ms;
                match_end   = me;
                match_la    = pre->lookahead[i];
            }
        }

        if (match_start >= subj_len) return false;
    }

    /* Convert back to absolute positions */
    size_t abs_start = it->offset + match_start;
    size_t abs_end   = it->offset + match_end;

    /* Strip lookahead byte: the last matched byte belongs to the next piece */
    size_t piece_end = match_la && abs_end > abs_start ? abs_end - 1 : abs_end;
    *out_start       = abs_start;
    *out_end         = piece_end;
    it->offset       = piece_end;
    return true;
}

void pretok_iter_free(PretokIter *it) {
    if (it && it->mdata) {
        pcre2_match_data_free(it->mdata);
        it->mdata = NULL;
    }
}

/* =========================================================================
 * encode / count helpers
 * ========================================================================= */

uint32_t *tokenizer_encode(const Tokenizer *tok,
                           const uint8_t *text, size_t text_len,
                           size_t *out_n) {
    if (!tok->pre) {
        /* No pretokenization */
        return bpe_encode_via_backtracking(tok->bpe, text, text_len, out_n);
    }

    /* Accumulate results from each piece */
    uint32_t *result   = NULL;
    size_t    count    = 0;
    size_t    result_cap = 256;
    result = (uint32_t *)malloc(result_cap * sizeof(uint32_t));

    PretokIter it = pretok_iter_new(tok->pre, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe <= ps) continue;
        size_t   piece_n;
        uint32_t *piece = bpe_encode_via_backtracking(tok->bpe, text + ps,
                                                      pe - ps, &piece_n);
        if (count + piece_n > result_cap) {
            while (count + piece_n > result_cap) result_cap *= 2;
            result = (uint32_t *)realloc(result, result_cap * sizeof(uint32_t));
        }
        memcpy(result + count, piece, piece_n * sizeof(uint32_t));
        count += piece_n;
        free(piece);
    }
    pretok_iter_free(&it);

    *out_n = count;
    return result;
}

size_t tokenizer_count(const Tokenizer *tok,
                       const uint8_t *text, size_t text_len) {
    if (!tok->pre) return bpe_count(tok->bpe, text, text_len);

    size_t total = 0;
    PretokIter it = pretok_iter_new(tok->pre, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe > ps) total += bpe_count(tok->bpe, text + ps, pe - ps);
    }
    pretok_iter_free(&it);
    return total;
}

size_t tokenizer_count_till_limit(const Tokenizer *tok,
                                   const uint8_t *text, size_t text_len,
                                   size_t token_limit) {
    if (!tok->pre)
        return bpe_count_till_limit(tok->bpe, text, text_len, token_limit);

    size_t total = 0;
    PretokIter it = pretok_iter_new(tok->pre, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe <= ps) continue;
        size_t remaining = (total < token_limit) ? token_limit - total : 0;
        size_t piece_count = bpe_count_till_limit(tok->bpe, text + ps,
                                                   pe - ps, remaining);
        if (piece_count == SIZE_MAX) {
            pretok_iter_free(&it);
            return SIZE_MAX;
        }
        total += piece_count;
        if (total > token_limit) {
            pretok_iter_free(&it);
            return SIZE_MAX;
        }
    }
    pretok_iter_free(&it);
    return total;
}
