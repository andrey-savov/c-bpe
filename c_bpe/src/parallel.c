/* parallel.c — OpenMP-accelerated batch encode / count.
 *
 * Falls back to sequential processing if OpenMP is unavailable.
 * Activated by passing compile flag -fopenmp / /openmp.
 */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#ifdef _OPENMP
#  include <omp.h>
#endif
#include "tokenizer.h"

/* =========================================================================
 * Batch encode
 * ========================================================================= */

/**
 * Encode `n` texts in parallel.
 *
 * @param tok        tokenizer (thread-safe: read-only after construction)
 * @param texts      array of n UTF-8 byte pointers
 * @param text_lens  array of n lengths
 * @param n          number of texts
 * @param out_tokens outputs[i] is a heap-allocated uint32_t array (caller frees)
 * @param out_ns     out_ns[i]  is the length of outputs[i]
 */
void parallel_encode_batch(const Tokenizer *tok,
                            const uint8_t **texts, const size_t *text_lens,
                            size_t n,
                            uint32_t **out_tokens, size_t *out_ns) {
    int i;
    int ni = (int)n;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 4)
#endif
    for (i = 0; i < ni; i++) {
        out_tokens[i] = tokenizer_encode(tok, texts[i], text_lens[i],
                                         &out_ns[i]);
    }
}

/**
 * Count tokens in `n` texts in parallel.
 *
 * @param out_counts  out_counts[i] = number of tokens in text i
 */
void parallel_count_batch(const Tokenizer *tok,
                           const uint8_t **texts, const size_t *text_lens,
                           size_t n, size_t *out_counts) {
    int i;
    int ni = (int)n;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 4)
#endif
    for (i = 0; i < ni; i++) {
        out_counts[i] = tokenizer_count(tok, texts[i], text_lens[i]);
    }
}

/**
 * Count tokens with limit for `n` texts in parallel.
 * out_counts[i] == SIZE_MAX means limit was exceeded for text i.
 */
void parallel_count_till_limit_batch(const Tokenizer *tok,
                                      const uint8_t **texts,
                                      const size_t *text_lens,
                                      size_t n, size_t token_limit,
                                      size_t *out_counts) {
    int i;
    int ni = (int)n;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 4)
#endif
    for (i = 0; i < ni; i++) {
        out_counts[i] = tokenizer_count_till_limit(tok, texts[i], text_lens[i],
                                                   token_limit);
    }
}
