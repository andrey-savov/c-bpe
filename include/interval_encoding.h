/* interval_encoding.h / interval_encoding.c
 * Port of bpe/src/interval_encoding.rs
 *
 * Precomputes prefix-tree metadata so that count(range) is typically O(1).
 */
#ifndef INTERVAL_ENCODING_H
#define INTERVAL_ENCODING_H

#include <stdint.h>
#include <stddef.h>
#include "bpe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IntervalEncoding {
    const BytePairEncoding *bpe;
    const uint8_t          *text;
    size_t                   text_len;
    uint32_t               *last_token; /* length = text_len */
    uint32_t               *tree_id;    /* length = text_len + 1 */
    uint32_t               *tree_end;   /* length = text_len + 1 */
    uint32_t               *tree_depth; /* length = text_len + 1 */
} IntervalEncoding;

/* Build the interval encoding from a BPE + text.
 * text pointer must remain valid for the lifetime of the returned object. */
IntervalEncoding *interval_encoding_new(const BytePairEncoding *bpe,
                                        const uint8_t *text, size_t text_len);

void interval_encoding_free(IntervalEncoding *ie);

/* Count tokens in text[start..end] in typically O(1) time. */
size_t interval_encoding_count(const IntervalEncoding *ie,
                                size_t start, size_t end);

#ifdef __cplusplus
}
#endif
#endif /* INTERVAL_ENCODING_H */
