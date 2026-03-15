/* test_bpe_core.c — Unit tests for bpe_core: BPE construction and encoding.
 *
 * Build (from c_bpe root, after running codegen to generate dict headers):
 *   gcc -O2 -Iinclude -Ibuild/codegen \
 *       tests/test_bpe_core.c src/ac_bpe.c src/bpe_core.c \
 *       -o test_bpe_core && ./test_bpe_core
 *
 * Note: This test builds a tiny hand-crafted dictionary so it doesn't need
 * the full tiktoken data files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bpe.h"

/* Simple harness */
static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if ((cond)) { \
            printf("  PASS: %s\n", (msg)); g_pass++; \
        } else { \
            printf("  FAIL: %s  (line %d)\n", (msg), __LINE__); g_fail++; \
        } \
    } while(0)

/* =========================================================================
 * Tiny dictionary: single-byte tokens 'a'..'z' (IDs 0..25) + "ab" (26),
 * "bc" (27), "abc" (28).
 *
 * Token layout: ALL_TOKENS = a b c ... z a b a b c
 *               TOKEN_STARTS = [0,1,2,...,25,26,28,30]
 * ========================================================================= */

static const uint8_t TINY_ALL_TOKENS[] = {
    /* 0-25: single bytes 'a'..'z' */
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    /* 26: "ab" */
    'a','b',
    /* 27: "bc" */
    'b','c',
    /* 28: "abc" */
    'a','b','c'
};

/* TOKEN_STARTS[i] = byte offset of token i.  Last entry = total length. */
static const uint32_t TINY_TOKEN_STARTS[] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
    26,28,30,33  /* 29 entries for 29 tokens (0..28) + sentinel */
};

#define TINY_NUM_TOKENS 29
/* Use FNV hash factor (same as production) */
#define TINY_HASH_FACTOR 17846336922010275747ULL

/* =========================================================================
 * Test: construction
 * ========================================================================= */

static void test_construction(void) {
    printf("\n[test_construction]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    CHECK(bpe != NULL, "bpe_from_dictionary returns non-NULL");
    if (bpe) bpe_free(bpe);
}

/* =========================================================================
 * Test: encode single-byte input
 * ========================================================================= */

static void test_encode_single_byte(void) {
    printf("\n[test_encode_single_byte]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP (construction failed)\n"); return; }

    size_t n;
    uint32_t *toks = bpe_encode_via_backtracking(
        bpe, (const uint8_t*)"a", 1, &n);
    CHECK(n == 1, "encode 'a' -> 1 token");
    /* 'a' is token 0 in our tiny dictionary */
    if (n == 1) CHECK(toks[0] == 0, "token id for 'a' is 0");
    free(toks);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: encode 2-byte pair — "ab" should be a single token
 * ========================================================================= */

static void test_encode_pair(void) {
    printf("\n[test_encode_pair]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    size_t n;
    uint32_t *toks = bpe_encode_via_backtracking(
        bpe, (const uint8_t*)"ab", 2, &n);
    CHECK(n == 1, "encode 'ab' -> 1 token (pair merge)");
    if (n == 1) CHECK(toks[0] == 26, "token id for 'ab' is 26");
    free(toks);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: encode "abc" — should yield token 28 or {26, 'c'} (depends on
 * merge priority).  Either way, verify decode(encode("abc")) == "abc".
 * ========================================================================= */

static void test_encode_decode_roundtrip(void) {
    printf("\n[test_encode_decode_roundtrip]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    const uint8_t input[] = "abc";
    size_t n;
    uint32_t *toks = bpe_encode_via_backtracking(bpe, input, 3, &n);
    CHECK(n >= 1, "encode 'abc' produces tokens");

    size_t dec_len;
    uint8_t *decoded = bpe_decode_tokens(bpe, toks, n, &dec_len);
    CHECK(dec_len == 3, "decoded length == 3");
    if (dec_len == 3)
        CHECK(memcmp(decoded, "abc", 3) == 0, "decode(encode('abc')) == 'abc'");
    free(toks);
    free(decoded);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: count
 * ========================================================================= */

static void test_count(void) {
    printf("\n[test_count]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    size_t c = bpe_count(bpe, (const uint8_t*)"abc", 3);
    size_t n;
    uint32_t *toks = bpe_encode_via_backtracking(bpe, (const uint8_t*)"abc", 3, &n);
    CHECK(c == n, "bpe_count == number of encode tokens");
    free(toks);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: encode_via_bitfield consistency with backtracking
 * ========================================================================= */

static void test_bitfield_vs_backtrack(void) {
    printf("\n[test_bitfield_vs_backtrack]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    const uint8_t text[] = "abcabc";
    size_t n1, n2;
    uint32_t *t1 = bpe_encode_via_backtracking(bpe, text, 6, &n1);
    uint32_t *t2 = bpe_encode_via_bitfield(bpe, text, 6, &n2);

    CHECK(n1 == n2, "bitfield and backtrack yield same token count");
    if (n1 == n2 && n1 > 0)
        CHECK(memcmp(t1, t2, n1 * sizeof(uint32_t)) == 0,
              "bitfield and backtrack yield identical tokens");
    free(t1);
    free(t2);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: encode_greedy — no pair tokens, just single-byte
 * ========================================================================= */

static void test_encode_greedy_chars(void) {
    printf("\n[test_encode_greedy_chars]\n");
    /* Use a dictionary with only single-byte tokens to ensure greedy is trivial */
    const uint8_t all[] = {'a','b','c'};
    const uint32_t starts[] = {0,1,2,3};
    BytePairEncoding *bpe = bpe_from_dictionary(all, starts, 3, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    size_t n;
    uint32_t *toks = bpe_encode_greedy(bpe, (const uint8_t*)"abc", 3, &n);
    CHECK(n == 3, "greedy produces 3 tokens for 'abc' with single-byte vocab");
    free(toks);
    bpe_free(bpe);
}

/* =========================================================================
 * Test: empty input
 * ========================================================================= */

static void test_empty_input(void) {
    printf("\n[test_empty_input]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    size_t n;
    uint32_t *toks = bpe_encode_via_backtracking(bpe, (const uint8_t*)"", 0, &n);
    CHECK(n == 0, "empty input -> 0 tokens");
    free(toks);

    size_t c = bpe_count(bpe, (const uint8_t*)"", 0);
    CHECK(c == 0, "count of empty input == 0");
    bpe_free(bpe);
}

/* =========================================================================
 * Test: is_valid_token_pair
 * ========================================================================= */

static void test_valid_token_pair(void) {
    printf("\n[test_valid_token_pair]\n");
    BytePairEncoding *bpe = bpe_from_dictionary(
        TINY_ALL_TOKENS, TINY_TOKEN_STARTS, TINY_NUM_TOKENS, TINY_HASH_FACTOR);
    if (!bpe) { printf("  SKIP\n"); return; }

    /* (token 0 'a', token 1 'b') should form a valid pair (token 26 "ab") */
    bool v = bpe_is_valid_token_pair(bpe, 0, 1);
    CHECK(v == true, "('a','b') is a valid pair");

    /* ('c','a') should NOT be a valid pair (no "ca" in dict) */
    bool inv = bpe_is_valid_token_pair(bpe, 2, 0);
    CHECK(inv == false, "('c','a') not a valid pair");
    bpe_free(bpe);
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== bpe_core unit tests ===\n");
    test_construction();
    test_encode_single_byte();
    test_encode_pair();
    test_encode_decode_roundtrip();
    test_count();
    test_bitfield_vs_backtrack();
    test_encode_greedy_chars();
    test_empty_input();
    test_valid_token_pair();
    printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
