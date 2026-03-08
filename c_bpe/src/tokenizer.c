/* tokenizer.c — Tokenizer with native pretokenizer.
 *
 * Uses hand-coded pretokenizer functions (pretok_cl100k / pretok_o200k).
 *
 * Replicates bpe-openai/src/lib.rs Tokenizer:
 *   - split text using pretokenizer
 *   - encode each piece with BytePairEncoding
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tokenizer.h"

/* =========================================================================
 * Tokenizer constructor / destructor
 * ========================================================================= */

Tokenizer *tokenizer_new_native(BytePairEncoding *bpe, pretok_fn fn) {
    Tokenizer *tok = (Tokenizer *)calloc(1, sizeof(Tokenizer));
    tok->bpe = bpe;
    tok->native_pre = fn;
    return tok;
}

void tokenizer_free(Tokenizer *tok) {
    if (!tok) return;
    free(tok);
}

/* =========================================================================
 * Pretokenizer iteration
 * ========================================================================= */

PretokIter pretok_iter_new_native_ext(pretok_fn fn,
                                      const uint8_t *text, size_t text_len) {
    PretokIter it;
    memset(&it, 0, sizeof(it));
    it.native   = fn;
    it.text     = text;
    it.text_len = text_len;
    it.offset   = 0;
    return it;
}

static PretokIter pretok_iter_new_for(const Tokenizer *tok,
                                      const uint8_t *text, size_t text_len) {
    if (tok->native_pre)
        return pretok_iter_new_native_ext(tok->native_pre, text, text_len);
    /* no pretokenizer */
    PretokIter it;
    memset(&it, 0, sizeof(it));
    it.text = text;
    it.text_len = text_len;
    return it;
}

bool pretok_iter_next(PretokIter *it, size_t *out_start, size_t *out_end) {
    if (!it->native || it->offset >= it->text_len) return false;

    PretokMatch m;
    if (!it->native(it->text, it->text_len, it->offset, &m))
        return false;

    size_t piece_end = m.end;
    if (m.lookahead && m.end > m.start) {
        /* Strip last UTF-8 codepoint */
        const uint8_t *p = it->text + m.start;
        const uint8_t *e = it->text + m.end;
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

void pretok_iter_free(PretokIter *it) {
    (void)it; /* nothing to free */
}

/* =========================================================================
 * encode / count helpers
 * ========================================================================= */

uint32_t *tokenizer_encode(const Tokenizer *tok,
                           const uint8_t *text, size_t text_len,
                           size_t *out_n) {
    if (!tok->native_pre) {
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
    if (!tok->native_pre) return bpe_count(tok->bpe, text, text_len);

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
    if (!tok->native_pre)
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
