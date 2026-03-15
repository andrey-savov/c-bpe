/* prependable_encoder.h / prependable_encoder.c
 * Port of bpe/src/prependable_encoder.rs
 *
 * Allows O(1) amortised token-count queries while prepending bytes.
 * Uses the reverse overlapping automaton.
 */
#ifndef PREPENDABLE_ENCODER_H
#define PREPENDABLE_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "bpe.h"
#include "ac_bpe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PrepState {
    int32_t  ac_state;   /* overlapping_searcher_rev state */
    uint32_t prev_token; /* first token in this suffix, UINT32_MAX if none */
    uint32_t count;
} PrepState;

typedef struct PrependableEncoder {
    const BytePairEncoding *bpe;
    PrepState               *states;
    size_t                   nstates;
    size_t                   cap;
} PrependableEncoder;

PrependableEncoder *prependable_encoder_new(const BytePairEncoding *bpe);
void                prependable_encoder_free(PrependableEncoder *enc);
void                prependable_encoder_push(PrependableEncoder *enc, uint8_t c);
void                prependable_encoder_extend(PrependableEncoder *enc,
                                               const uint8_t *bytes, size_t len);
void                prependable_encoder_truncate(PrependableEncoder *enc, size_t len);
size_t              prependable_encoder_token_count(const PrependableEncoder *enc);
size_t              prependable_encoder_len(const PrependableEncoder *enc);

#ifdef __cplusplus
}
#endif
#endif /* PREPENDABLE_ENCODER_H */
