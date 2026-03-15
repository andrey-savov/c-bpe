/* prependable_encoder.c — Port of bpe/src/prependable_encoder.rs */
#include <stdlib.h>
#include <string.h>
#include "prependable_encoder.h"

PrependableEncoder *prependable_encoder_new(const BytePairEncoding *bpe) {
    PrependableEncoder *enc = (PrependableEncoder *)calloc(1, sizeof(PrependableEncoder));
    enc->bpe    = bpe;
    enc->cap    = 256;
    enc->states = (PrepState *)malloc(enc->cap * sizeof(PrepState));
    return enc;
}

void prependable_encoder_free(PrependableEncoder *enc) {
    if (!enc) return;
    free(enc->states);
    free(enc);
}

/* Each call to push() adds the new byte to the FRONT of the logical string.
 * Internally we reverse the sequence using the rev automaton. */
void prependable_encoder_push(PrependableEncoder *enc, uint8_t c) {
    if (enc->nstates == enc->cap) {
        enc->cap *= 2;
        enc->states = (PrepState *)realloc(enc->states,
                                           enc->cap * sizeof(PrepState));
    }

    int32_t prev_state = (enc->nstates > 0) ? enc->states[enc->nstates - 1].ac_state
                                            : ac_start_state();

    AcMatchIter it;
    it.automaton   = enc->bpe->overlapping_searcher_rev;
    it.state       = prev_state;
    it.pos         = enc->nstates;
    it.pending_out = 0;
    it.pending_end = 0;

    ac_iter_advance(&it, c, enc->nstates + 1);

    uint32_t new_tok = UINT32_MAX;
    uint32_t new_cnt = 0;
    uint32_t out_tok;
    size_t   out_start;

    while (ac_iter_next_match(&it, &out_tok, &out_start)) {
        if (out_start == 0) {
            new_tok = out_tok;
            new_cnt = 1;
            break;
        } else {
            uint32_t next_tok = enc->states[out_start - 1].prev_token;
            /* For prependable, validate (new_token, next_token) pair */
            if (bpe_is_valid_token_pair(enc->bpe, out_tok, next_tok)) {
                new_tok = out_tok;
                new_cnt = enc->states[out_start - 1].count + 1;
                break;
            }
        }
    }

    enc->states[enc->nstates++] = (PrepState){
        .ac_state   = it.state,
        .prev_token = new_tok,
        .count      = new_cnt,
    };
}

void prependable_encoder_extend(PrependableEncoder *enc,
                                 const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) prependable_encoder_push(enc, bytes[i]);
}

void prependable_encoder_truncate(PrependableEncoder *enc, size_t len) {
    if (len < enc->nstates) enc->nstates = len;
}

size_t prependable_encoder_token_count(const PrependableEncoder *enc) {
    if (enc->nstates == 0) return 0;
    return (size_t)enc->states[enc->nstates - 1].count;
}

size_t prependable_encoder_len(const PrependableEncoder *enc) {
    return enc->nstates;
}
