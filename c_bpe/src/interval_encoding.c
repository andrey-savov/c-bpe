/* interval_encoding.c — Port of bpe/src/interval_encoding.rs */
#include <stdlib.h>
#include <string.h>
#include "interval_encoding.h"

IntervalEncoding *interval_encoding_new(const BytePairEncoding *bpe,
                                        const uint8_t *text, size_t text_len) {
    IntervalEncoding *ie = (IntervalEncoding *)calloc(1, sizeof(IntervalEncoding));
    ie->bpe      = bpe;
    ie->text     = text;
    ie->text_len = text_len;

    /* Build last_token via encode_all_prefixes */
    ie->last_token = bpe_encode_all_prefixes(bpe, text, text_len);

    /* Compute subtree sizes bottom-up */
    uint32_t *tree_size = (uint32_t *)malloc((text_len + 1) * sizeof(uint32_t));
    for (size_t i = 0; i <= text_len; i++) tree_size[i] = 1;

    for (size_t id = text_len; id > 0; id--) {
        uint32_t tok    = ie->last_token[id - 1];
        size_t   parent = id - bpe_token_len(bpe, tok);
        tree_size[parent] += tree_size[id];
    }

    ie->tree_id    = (uint32_t *)malloc((text_len + 1) * sizeof(uint32_t));
    ie->tree_end   = (uint32_t *)malloc((text_len + 1) * sizeof(uint32_t));
    ie->tree_depth = (uint32_t *)malloc((text_len + 1) * sizeof(uint32_t));

    /* Root (index 0) is the empty prefix */
    ie->tree_id[0]    = 0;
    ie->tree_end[0]   = 1;
    ie->tree_depth[0] = 0;

    for (size_t id = 1; id <= text_len; id++) {
        uint32_t tok    = ie->last_token[id - 1];
        size_t   parent = id - bpe_token_len(bpe, tok);

        ie->tree_id[id]    = ie->tree_end[parent];
        ie->tree_end[id]   = ie->tree_end[parent] + 1;
        ie->tree_depth[id] = ie->tree_depth[parent] + 1;
        ie->tree_end[parent] += tree_size[id];
    }

    free(tree_size);
    return ie;
}

void interval_encoding_free(IntervalEncoding *ie) {
    if (!ie) return;
    free(ie->last_token);
    free(ie->tree_id);
    free(ie->tree_end);
    free(ie->tree_depth);
    free(ie);
}

/* =========================================================================
 * count(start, end) — internal backtrack helper
 * ========================================================================= */

/* We need a mini backtrack encoder that works on a sub-slice but lets us
 * bail out early when compatible with the precomputed tree. */

typedef struct {
    const BytePairEncoding *bpe;
    const uint8_t          *text;
    size_t                   text_len;
    uint32_t               *tokens;
    size_t                   token_cap;
    size_t                   token_count;
    uint32_t                 next_token;
    size_t                   pos;
    /* bitfield: use a small stack-allocated version for tiny texts,
     * heap otherwise */
    uint64_t                *bf_words;
    size_t                   bf_nwords;
} MiniEnc;

static void bf_init(MiniEnc *e) {
    e->bf_nwords = (e->text_len + 64) / 64;
    e->bf_words  = (uint64_t *)malloc(e->bf_nwords * sizeof(uint64_t));
    for (size_t i = 0; i < e->bf_nwords; i++)
        e->bf_words[i] = ~(uint64_t)0;
}
static void bf_free(MiniEnc *e) { free(e->bf_words); }
static int bf_is_set(MiniEnc *e, size_t bit) {
    return (e->bf_words[bit / 64] >> (bit % 64)) & 1;
}
static void bf_clear(MiniEnc *e, size_t bit) {
    e->bf_words[bit / 64] &= ~((uint64_t)1 << (bit % 64));
}

static MiniEnc mini_enc_new(const BytePairEncoding *bpe,
                             const uint8_t *text, size_t len, size_t cap) {
    MiniEnc e;
    e.bpe         = bpe;
    e.text        = text;
    e.text_len    = len;
    e.token_cap   = cap > 8 ? cap : 8;
    e.tokens      = (uint32_t *)malloc(e.token_cap * sizeof(uint32_t));
    e.token_count = 0;
    e.pos         = 0;
    e.next_token  = bpe_next_match(bpe, text, len);
    bf_init(&e);
    return e;
}

static void mini_enc_free(MiniEnc *e) {
    free(e->tokens);
    bf_free(e);
}

static uint32_t mini_step(MiniEnc *e) {
    if (e->next_token == UINT32_MAX) return UINT32_MAX;
    uint32_t token = e->next_token;
    uint32_t last  = e->token_count > 0 ? e->tokens[e->token_count - 1]
                                        : UINT32_MAX;
    for (;;) {
        uint32_t tlen    = bpe_token_len(e->bpe, token);
        size_t   end_pos = e->pos + tlen;

        int pair_ok = (last == UINT32_MAX) ||
                      bpe_is_valid_token_pair(e->bpe, last, token);

        if (bf_is_set(e, end_pos) && pair_ok) {
            if (e->token_count == e->token_cap) {
                e->token_cap *= 2;
                e->tokens = (uint32_t *)realloc(e->tokens,
                                                e->token_cap * sizeof(uint32_t));
            }
            e->tokens[e->token_count++] = token;
            e->pos = end_pos;
            e->next_token = bpe_next_match(e->bpe, e->text + end_pos,
                                           e->text_len - end_pos);
            break;
        } else {
            uint32_t shorter = bpe_next_prefix(e->bpe, token);
            if (shorter != UINT32_MAX) {
                token = shorter;
            } else {
                bf_clear(e, e->pos);
                if (e->token_count == 0) {
                    e->next_token = UINT32_MAX;
                    break;
                }
                e->token_count--;
                uint32_t popped = e->tokens[e->token_count];
                e->pos  -= bpe_token_len(e->bpe, popped);
                last     = e->token_count > 0 ? e->tokens[e->token_count - 1]
                                              : UINT32_MAX;
                e->next_token = popped;
                break;
            }
        }
    }
    return e->next_token;
}

size_t interval_encoding_count(const IntervalEncoding *ie,
                                size_t start, size_t end) {
    uint32_t leaf = ie->tree_id[end];

    MiniEnc e = mini_enc_new(ie->bpe, ie->text + start, end - start, 8);

    while (mini_step(&e) != UINT32_MAX) {
        if (e.token_count > 0) {
            uint32_t prev_token = e.tokens[e.token_count - 1];
            size_t   end_pos_abs = e.pos + start;

            if (end_pos_abs > 0 && end_pos_abs <= ie->text_len) {
                uint32_t next_tok = e.next_token;
                if (next_tok == UINT32_MAX) break;

                uint32_t nxt_abs_end = (uint32_t)(end_pos_abs + bpe_token_len(ie->bpe, next_tok));
                if (nxt_abs_end > ie->text_len) break;

                if ((ie->tree_id[end_pos_abs] <= leaf &&
                     leaf < ie->tree_end[end_pos_abs]) &&
                    ie->last_token[end_pos_abs - 1] == prev_token &&
                    ie->last_token[nxt_abs_end - 1] == next_tok) {
                    size_t result = e.token_count +
                        (ie->tree_depth[end] - ie->tree_depth[end_pos_abs]);
                    mini_enc_free(&e);
                    return result;
                }
            }
        }
    }

    size_t result = e.token_count;
    mini_enc_free(&e);
    return result;
}
