/* tokenizer.c — Pretokenizer + Tokenizer.
 *
 * Supports two pretokenizer backends:
 *   1. Native (hand-coded): pretok_cl100k / pretok_o200k function pointers
 *   2. PCRE2 regex-based (legacy, guarded by #ifndef C_BPE_NO_PCRE2)
 *
 * Replicates bpe-openai/src/lib.rs Tokenizer:
 *   - split text using pretokenizer
 *   - encode each piece with BytePairEncoding
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tokenizer.h"

#ifndef C_BPE_NO_PCRE2
/* =========================================================================
 * PCRE2 Build helpers
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
        pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    }
    return re;
}

/* =========================================================================
 * Pretokenizer (PCRE2)
 * ========================================================================= */

Pretokenizer *pretokenizer_new(const char **patterns, const bool *lookaheads,
                               int npatterns, int *error_code,
                               size_t *error_offset) {
    if (npatterns > PRETOK_MAX_PATTERNS) return NULL;
    Pretokenizer *pre = (Pretokenizer *)calloc(1, sizeof(Pretokenizer));
    for (int i = 0; i < npatterns; i++) {
        pre->patterns[i] = compile_pat(patterns[i], error_code, error_offset);
        if (!pre->patterns[i]) {
            for (int j = 0; j < i; j++) pcre2_code_free(pre->patterns[j]);
            free(pre);
            return NULL;
        }
        pre->lookahead[i] = lookaheads ? lookaheads[i] : false;
    }
    pre->npatterns = npatterns;

    /* Build combined pattern: (pat0)|(pat1)|(pat2)... */
    size_t total_len = 0;
    for (int i = 0; i < npatterns; i++)
        total_len += strlen(patterns[i]) + 4;
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

    return pre;
}

void pretokenizer_free(Pretokenizer *pre) {
    if (!pre) return;
    for (int i = 0; i < pre->npatterns; i++)
        if (pre->patterns[i]) pcre2_code_free(pre->patterns[i]);
    if (pre->combined) pcre2_code_free(pre->combined);
    free(pre);
}

#endif /* !C_BPE_NO_PCRE2 */

/* =========================================================================
 * Tokenizer constructors
 * ========================================================================= */

#ifndef C_BPE_NO_PCRE2
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
#endif /* !C_BPE_NO_PCRE2 */

Tokenizer *tokenizer_new_native(BytePairEncoding *bpe, pretok_fn fn) {
    Tokenizer *tok = (Tokenizer *)calloc(1, sizeof(Tokenizer));
    tok->bpe = bpe;
    tok->native_pre = fn;
    return tok;
}

void tokenizer_free(Tokenizer *tok) {
    if (!tok) return;
#ifndef C_BPE_NO_PCRE2
    pretokenizer_free(tok->pre);
#endif
    free(tok);
}

/* =========================================================================
 * Pretokenizer iteration — native path
 * ========================================================================= */

static PretokIter pretok_iter_new_native(pretok_fn fn,
                                         const uint8_t *text, size_t text_len) {
    PretokIter it;
    memset(&it, 0, sizeof(it));
    it.native   = fn;
    it.text     = text;
    it.text_len = text_len;
    it.offset   = 0;
    return it;
}

static bool pretok_iter_next_native(PretokIter *it,
                                    size_t *out_start, size_t *out_end) {
    if (!it->native || it->offset >= it->text_len) return false;

    PretokMatch m;
    if (!it->native(it->text, it->text_len, it->offset, &m))
        return false;

    size_t piece_end = m.end;
    if (m.lookahead && m.end > m.start) {
        /* Strip last UTF-8 codepoint */
        const uint8_t *p = it->text + m.start;
        const uint8_t *e = it->text + m.end;
        /* Walk backwards to find start of last codepoint */
        const uint8_t *last = e - 1;
        while (last > p && (*last & 0xC0) == 0x80)
            last--;
        piece_end = (size_t)(last - it->text);
    }

    *out_start = m.start;
    *out_end   = piece_end;
    it->offset = piece_end;
    return true;
}

/* =========================================================================
 * Pretokenizer iteration — PCRE2 path
 * ========================================================================= */

#ifndef C_BPE_NO_PCRE2

static PretokIter pretok_iter_new_pcre2(const Pretokenizer *pre,
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

static bool pretok_iter_next_pcre2(PretokIter *it,
                                   size_t *out_start, size_t *out_end) {
    const Pretokenizer *pre = it->pre;
    if (!pre || it->offset >= it->text_len) return false;

    const PCRE2_SPTR subj     = (PCRE2_SPTR)(it->text + it->offset);
    const PCRE2_SIZE subj_len = (PCRE2_SIZE)(it->text_len - it->offset);

    size_t match_start, match_end;
    bool   match_la = false;

    if (pre->combined) {
        int rc = pcre2_jit_match(pre->combined,
                                 subj, subj_len, 0,
                                 PCRE2_NOTEMPTY | PCRE2_ANCHORED,
                                 it->mdata, NULL);
        if (rc < 0) {
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

        for (int i = 0; i < pre->npatterns; i++) {
            if (ov[2*(i+1)] != PCRE2_UNSET) {
                match_la = pre->lookahead[i];
                break;
            }
        }
    } else {
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

    size_t abs_start = it->offset + match_start;
    size_t abs_end   = it->offset + match_end;
    size_t piece_end = match_la && abs_end > abs_start ? abs_end - 1 : abs_end;
    *out_start       = abs_start;
    *out_end         = piece_end;
    it->offset       = piece_end;
    return true;
}

#endif /* !C_BPE_NO_PCRE2 */

/* =========================================================================
 * Unified iteration API
 * ========================================================================= */

PretokIter pretok_iter_new(const Pretokenizer *pre,
                           const uint8_t *text, size_t text_len) {
    /* Legacy API — only for PCRE2 path */
    PretokIter it;
    memset(&it, 0, sizeof(it));
#ifndef C_BPE_NO_PCRE2
    return pretok_iter_new_pcre2(pre, text, text_len);
#else
    it.text = text;
    it.text_len = text_len;
    return it;
#endif
}

static PretokIter pretok_iter_new_for(const Tokenizer *tok,
                                      const uint8_t *text, size_t text_len) {
    if (tok->native_pre)
        return pretok_iter_new_native(tok->native_pre, text, text_len);
#ifndef C_BPE_NO_PCRE2
    if (tok->pre)
        return pretok_iter_new_pcre2(tok->pre, text, text_len);
#endif
    /* no pretokenizer */
    PretokIter it;
    memset(&it, 0, sizeof(it));
    it.text = text;
    it.text_len = text_len;
    return it;
}

bool pretok_iter_next(PretokIter *it, size_t *out_start, size_t *out_end) {
    if (it->native)
        return pretok_iter_next_native(it, out_start, out_end);
#ifndef C_BPE_NO_PCRE2
    if (it->pre)
        return pretok_iter_next_pcre2(it, out_start, out_end);
#endif
    return false;
}

void pretok_iter_free(PretokIter *it) {
#ifndef C_BPE_NO_PCRE2
    if (it && it->mdata) {
        pcre2_match_data_free(it->mdata);
        it->mdata = NULL;
    }
#endif
}

/* =========================================================================
 * encode / count helpers
 * ========================================================================= */

static bool has_pretokenizer(const Tokenizer *tok) {
    if (tok->native_pre) return true;
#ifndef C_BPE_NO_PCRE2
    if (tok->pre) return true;
#endif
    return false;
}

uint32_t *tokenizer_encode(const Tokenizer *tok,
                           const uint8_t *text, size_t text_len,
                           size_t *out_n) {
    if (!has_pretokenizer(tok)) {
        return bpe_encode_via_backtracking(tok->bpe, text, text_len, out_n);
    }

    uint32_t *result   = NULL;
    size_t    count    = 0;
    size_t    result_cap = 256;
    result = (uint32_t *)malloc(result_cap * sizeof(uint32_t));

    BpeEncScratch *scratch = bpe_scratch_new();
    PretokIter it = pretok_iter_new_for(tok, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe <= ps) continue;
        size_t   piece_n;
        const uint32_t *piece = bpe_encode_piece(tok->bpe, text + ps,
                                                  pe - ps, scratch, &piece_n);
        if (count + piece_n > result_cap) {
            while (count + piece_n > result_cap) result_cap *= 2;
            result = (uint32_t *)realloc(result, result_cap * sizeof(uint32_t));
        }
        memcpy(result + count, piece, piece_n * sizeof(uint32_t));
        count += piece_n;
    }
    pretok_iter_free(&it);
    bpe_scratch_free(scratch);

    *out_n = count;
    return result;
}

size_t tokenizer_count(const Tokenizer *tok,
                       const uint8_t *text, size_t text_len) {
    if (!has_pretokenizer(tok)) return bpe_count(tok->bpe, text, text_len);

    size_t total = 0;
    BpeEncScratch *scratch = bpe_scratch_new();
    PretokIter it = pretok_iter_new_for(tok, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe > ps) total += bpe_count_piece(tok->bpe, text + ps,
                                               pe - ps, scratch);
    }
    pretok_iter_free(&it);
    bpe_scratch_free(scratch);
    return total;
}

size_t tokenizer_count_till_limit(const Tokenizer *tok,
                                   const uint8_t *text, size_t text_len,
                                   size_t token_limit) {
    if (!has_pretokenizer(tok))
        return bpe_count_till_limit(tok->bpe, text, text_len, token_limit);

    size_t total = 0;
    BpeEncScratch *scratch = bpe_scratch_new();
    PretokIter it = pretok_iter_new_for(tok, text, text_len);
    size_t ps, pe;
    while (pretok_iter_next(&it, &ps, &pe)) {
        if (pe <= ps) continue;
        size_t remaining = (total < token_limit) ? token_limit - total : 0;
        size_t piece_count = bpe_count_till_limit(tok->bpe, text + ps,
                                                   pe - ps, remaining);
        if (piece_count == SIZE_MAX) {
            pretok_iter_free(&it);
            bpe_scratch_free(scratch);
            return SIZE_MAX;
        }
        total += piece_count;
        if (total > token_limit) {
            pretok_iter_free(&it);
            bpe_scratch_free(scratch);
            return SIZE_MAX;
        }
    }
    pretok_iter_free(&it);
    bpe_scratch_free(scratch);
    return total;
}
