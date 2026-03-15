/* appendable_encoder.c — Port of bpe/src/appendable_encoder.rs */
#include <stdlib.h>
#include <string.h>
#include "appendable_encoder.h"

AppendableEncoder *appendable_encoder_new(const BytePairEncoding *bpe) {
    AppendableEncoder *enc = (AppendableEncoder *)calloc(1, sizeof(AppendableEncoder));
    enc->bpe  = bpe;
    enc->cap  = 256;
    enc->states = (AppState *)malloc(enc->cap * sizeof(AppState));
    return enc;
}

void appendable_encoder_free(AppendableEncoder *enc) {
    if (!enc) return;
    free(enc->states);
    free(enc);
}

void appendable_encoder_push(AppendableEncoder *enc, uint8_t c) {
    if (enc->nstates == enc->cap) {
        enc->cap *= 2;
        enc->states = (AppState *)realloc(enc->states, enc->cap * sizeof(AppState));
    }

    int32_t prev_state = (enc->nstates > 0) ? enc->states[enc->nstates - 1].ac_state
                                            : ac_start_state();

    AcMatchIter it;
    it.automaton   = enc->bpe->overlapping_searcher;
    it.state       = prev_state;
    it.pos         = enc->nstates;
    it.pending_out = 0;
    it.pending_end = 0;

    ac_iter_advance(&it, c, enc->nstates + 1);

    uint32_t new_tok  = UINT32_MAX;
    uint32_t new_cnt  = 0;
    uint32_t out_tok;
    size_t   out_start;

    while (ac_iter_next_match(&it, &out_tok, &out_start)) {
        if (out_start == 0) {
            new_tok = out_tok;
            new_cnt = 1;
            break;
        } else {
            uint32_t prev_tok = enc->states[out_start - 1].last_token;
            if (bpe_is_valid_token_pair(enc->bpe, prev_tok, out_tok)) {
                new_tok = out_tok;
                new_cnt = enc->states[out_start - 1].count + 1;
                break;
            }
        }
    }

    enc->states[enc->nstates++] = (AppState){
        .ac_state   = it.state,
        .last_token = new_tok,
        .count      = new_cnt,
    };
}

void appendable_encoder_extend(AppendableEncoder *enc,
                                const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) appendable_encoder_push(enc, bytes[i]);
}

void appendable_encoder_truncate(AppendableEncoder *enc, size_t len) {
    if (len < enc->nstates) enc->nstates = len;
}

size_t appendable_encoder_token_count(const AppendableEncoder *enc) {
    if (enc->nstates == 0) return 0;
    return (size_t)enc->states[enc->nstates - 1].count;
}

size_t appendable_encoder_len(const AppendableEncoder *enc) {
    return enc->nstates;
}
