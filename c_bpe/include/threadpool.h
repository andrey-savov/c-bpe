#pragma once
/* threadpool.h — Work-queue thread pool for BPE batch operations.
 *
 * Workers block on a condition variable when the queue is empty and wake
 * only when items are pushed.  Each batch tracks completion via an atomic
 * counter + condvar, so the caller blocks until every item is done.
 * No busy-spinning between calls — idle workers consume zero CPU.
 */
#include <stddef.h>
#include <stdint.h>
#include "tokenizer.h"

typedef struct BpePool BpePool;

/* Create pool with `nthreads` worker threads.
 * Pass 0 to auto-detect hardware concurrency. */
BpePool *bpe_pool_create(int nthreads);
void     bpe_pool_destroy(BpePool *pool);

/* Number of worker threads in the pool. */
int bpe_pool_nthreads(const BpePool *pool);

void bpe_pool_encode_batch(
    BpePool *pool, const Tokenizer *tok,
    const uint8_t **texts, const size_t *text_lens, size_t n,
    uint32_t **out_tokens, size_t *out_ns);

void bpe_pool_count_batch(
    BpePool *pool, const Tokenizer *tok,
    const uint8_t **texts, const size_t *text_lens, size_t n,
    size_t *out_counts);

void bpe_pool_count_till_limit_batch(
    BpePool *pool, const Tokenizer *tok,
    const uint8_t **texts, const size_t *text_lens, size_t n,
    size_t token_limit, size_t *out_counts);
