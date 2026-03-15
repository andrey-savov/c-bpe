/* appendable_encoder.h / appendable_encoder.c
 * Port of bpe/src/appendable_encoder.rs
 *
 * Allows O(1) amortised token-count queries while appending bytes.
 */
#ifndef APPENDABLE_ENCODER_H
#define APPENDABLE_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "bpe.h"
#include "ac_bpe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AppState {
    int32_t  ac_state;    /* overlapping_searcher state */
    uint32_t last_token;  /* UINT32_MAX if none */
    uint32_t count;       /* tokens from start to this position */
} AppState;

typedef struct AppendableEncoder {
    const BytePairEncoding *bpe;
    AppState                *states; /* one per byte appended */
    size_t                   nstates;
    size_t                   cap;
} AppendableEncoder;

AppendableEncoder *appendable_encoder_new(const BytePairEncoding *bpe);
void               appendable_encoder_free(AppendableEncoder *enc);
void               appendable_encoder_push(AppendableEncoder *enc, uint8_t c);
void               appendable_encoder_extend(AppendableEncoder *enc,
                                             const uint8_t *bytes, size_t len);
void               appendable_encoder_truncate(AppendableEncoder *enc, size_t len);
size_t             appendable_encoder_token_count(const AppendableEncoder *enc);
size_t             appendable_encoder_len(const AppendableEncoder *enc);

#ifdef __cplusplus
}
#endif
#endif /* APPENDABLE_ENCODER_H */
