/* test_ac_bpe.c — Unit tests for the Double Array Aho-Corasick implementation.
 *
 * Build (from c_bpe root):
 *   gcc -O2 -Iinclude tests/test_ac_bpe.c src/ac_bpe.c -o test_ac_bpe && ./test_ac_bpe
 *   cl /O2 /Iinclude tests/test_ac_bpe.c src/ac_bpe.c /Fe:test_ac_bpe.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac_bpe.h"

/* Simple test harness */
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
 * Helper: build automaton for a fixed set of patterns
 * ========================================================================= */

static AcAutomaton *build_ll(const char **patterns, const uint32_t *ids, size_t n) {
    AcBuilder *b = ac_builder_new(AC_KIND_LEFTMOST_LONGEST);
    for (size_t i = 0; i < n; i++)
        ac_builder_add(b, (const uint8_t *)patterns[i], strlen(patterns[i]), ids[i]);
    AcAutomaton *a = ac_builder_build(b);
    ac_builder_free(b);
    return a;
}

static AcAutomaton *build_ov(const char **patterns, const uint32_t *ids, size_t n) {
    AcBuilder *b = ac_builder_new(AC_KIND_OVERLAPPING_FWD);
    for (size_t i = 0; i < n; i++)
        ac_builder_add(b, (const uint8_t *)patterns[i], strlen(patterns[i]), ids[i]);
    AcAutomaton *a = ac_builder_build(b);
    ac_builder_free(b);
    return a;
}

/* =========================================================================
 * Test: single-character tokens via leftmost-longest
 * ========================================================================= */

static void test_single_char(void) {
    printf("\n[test_single_char]\n");
    const char *pats[] = { "a", "b", "c" };
    const uint32_t ids[] = { 1, 2, 3 };
    AcAutomaton *a = build_ll(pats, ids, 3);
    CHECK(a != NULL, "automaton built");

    /* leftmost-longest advances pos by pos finding one match each call */
    const uint8_t *text = (const uint8_t *)"abc";
    size_t pos = 0, n = 3;
    int matches = 0;
    uint32_t toks[3]; size_t ends[3];
    while (pos < n) {
        uint32_t tok; size_t end;
        if (!ac_leftmost_longest(a, text + pos, n - pos, &tok, &end)) break;
        toks[matches] = tok;
        ends[matches] = end;
        matches++;
        pos += end;
    }
    CHECK(matches == 3, "3 matches scanning 'abc'");
    if (matches >= 1) CHECK(toks[0] == 1, "match[0] == 'a' (1)");
    if (matches >= 2) CHECK(toks[1] == 2, "match[1] == 'b' (2)");
    if (matches >= 3) CHECK(toks[2] == 3, "match[2] == 'c' (3)");
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: longer token beats shorter (leftmost-longest)
 * ========================================================================= */

static void test_longer_wins(void) {
    printf("\n[test_longer_wins]\n");
    const char *pats[] = { "a", "ab", "b" };
    const uint32_t ids[] = { 10, 20, 30 };
    AcAutomaton *a = build_ll(pats, ids, 3);
    CHECK(a != NULL, "automaton built");

    uint32_t tok; size_t end;
    bool found = ac_leftmost_longest(a, (const uint8_t *)"ab", 2, &tok, &end);
    CHECK(found, "found a match");
    if (found) {
        CHECK(tok == 20, "matched 'ab' (token 20)");
        CHECK(end == 2, "match end == 2");
    }
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: no match
 * ========================================================================= */

static void test_no_match(void) {
    printf("\n[test_no_match]\n");
    const char *pats[] = { "xyz" };
    const uint32_t ids[] = { 99 };
    AcAutomaton *a = build_ll(pats, ids, 1);

    uint32_t tok; size_t end;
    bool found = ac_leftmost_longest(a, (const uint8_t *)"abc", 3, &tok, &end);
    CHECK(!found, "no match when pattern absent");
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: overlapping iterator finds all occurrences
 * ========================================================================= */

static void test_overlapping_iter(void) {
    printf("\n[test_overlapping_iter]\n");
    const char *pats[] = { "a" };
    const uint32_t ids[] = { 1 };
    AcAutomaton *a = build_ov(pats, ids, 1);

    /* "ababa" has 'a' at positions 0, 2, 4 */
    const uint8_t *text = (const uint8_t *)"ababa";
    AcMatchIter it = ac_iter_new(a);
    int found = 0;
    for (size_t i = 0; i < 5; i++) {
        ac_iter_advance(&it, text[i], i + 1);
        uint32_t tok; size_t start;
        while (ac_iter_next_match(&it, &tok, &start)) found++;
    }
    CHECK(found == 3, "iterator finds 3 occurrences of 'a' in 'ababa'");
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: empty string input
 * ========================================================================= */

static void test_empty_input(void) {
    printf("\n[test_empty_input]\n");
    const char *pats[] = { "a" };
    const uint32_t ids[] = { 7 };
    AcAutomaton *a = build_ll(pats, ids, 1);

    uint32_t tok; size_t end;
    bool found = ac_leftmost_longest(a, (const uint8_t *)"", 0, &tok, &end);
    CHECK(!found, "no match on empty input");
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: shared prefixes (trie compression)
 * ========================================================================= */

static void test_shared_prefix(void) {
    printf("\n[test_shared_prefix]\n");
    const char *pats[] = { "he", "her", "here", "hello" };
    const uint32_t ids[] = { 1, 2, 3, 4 };
    AcAutomaton *a = build_ll(pats, ids, 4);
    CHECK(a != NULL, "automaton with shared prefixes built");

    uint32_t tok; size_t end;
    bool found = ac_leftmost_longest(a, (const uint8_t *)"here", 4, &tok, &end);
    CHECK(found, "found match");
    if (found) {
        CHECK(tok == 3, "'here' matched token 3");
        CHECK(end == 4, "match end == 4");
    }
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: binary / multi-byte tokens
 * ========================================================================= */

static void test_binary_tokens(void) {
    printf("\n[test_binary_tokens]\n");
    const uint8_t pat[] = { 0xC3, 0xA9 }; /* UTF-8 U+00E9 'e' with accent */
    AcBuilder *b = ac_builder_new(AC_KIND_LEFTMOST_LONGEST);
    ac_builder_add(b, pat, 2, 42);
    AcAutomaton *a = ac_builder_build(b);
    ac_builder_free(b);
    CHECK(a != NULL, "binary-token automaton built");

    /* Two occurrences in 4 bytes */
    const uint8_t text[] = { 0xC3, 0xA9, 0xC3, 0xA9 };
    int matches = 0;
    size_t pos = 0;
    while (pos < 4) {
        uint32_t tok; size_t end;
        if (!ac_leftmost_longest(a, text + pos, 4 - pos, &tok, &end)) break;
        if (tok == 42) matches++;
        pos += end;
    }
    CHECK(matches == 2, "found 2 occurrences of the binary token");
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: overlapping forward iterator — multiple patterns
 * ========================================================================= */

static void test_overlap_multi_pattern(void) {
    printf("\n[test_overlap_multi_pattern]\n");
    /* Both "a" and "ab" match at position 0 of "ab" */
    const char *pats[] = { "a", "ab" };
    const uint32_t ids[] = { 1, 2 };
    AcAutomaton *a = build_ov(pats, ids, 2);

    AcMatchIter it = ac_iter_new(a);
    const uint8_t *text = (const uint8_t *)"ab";
    int found = 0;
    for (size_t i = 0; i < 2; i++) {
        ac_iter_advance(&it, text[i], i + 1);
        uint32_t tok; size_t start;
        while (ac_iter_next_match(&it, &tok, &start)) found++;
    }
    /* Should find at least "a" (at pos 0 end 1) and "ab" (at pos 0 end 2) */
    CHECK(found >= 2, "overlapping: found 'a' and 'ab' in 'ab'");
    ac_automaton_free(a);
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== ac_bpe unit tests ===\n");
    test_single_char();
    test_longer_wins();
    test_no_match();
    test_overlapping_iter();
    test_empty_input();
    test_shared_prefix();
    test_binary_tokens();
    test_overlap_multi_pattern();
    printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ac_bpe.h"

/* Simple test harness */
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
 * Helper: build automaton for a fixed set of patterns
 * ========================================================================= */

static AcAutomaton *build(const char **patterns, const uint32_t *token_ids,
                           size_t n) {
    AcBuilder *b = ac_builder_new();
    for (size_t i = 0; i < n; i++)
        ac_builder_add(b, (const uint8_t *)patterns[i],
                       strlen(patterns[i]), token_ids[i]);
    AcAutomaton *a = ac_builder_build(b);
    ac_builder_free(b);
    return a;
}

/* =========================================================================
 * Test: single-character tokens
 * ========================================================================= */

static void test_single_char(void) {
    printf("\n[test_single_char]\n");
    const char *pats[] = { "a", "b", "c" };
    const uint32_t ids[] = { 1, 2, 3 };
    AcAutomaton *a = build(pats, ids, 3);
    CHECK(a != NULL, "automaton built");

    /* leftmost-longest on "abc" should yield 3 matches */
    const uint8_t text[] = "abc";
    size_t n_matches = 0;
    AcMatch *matches = ac_leftmost_longest(a, text, 3, &n_matches);
    CHECK(n_matches == 3, "3 matches in 'abc'");
    if (n_matches == 3) {
        CHECK(matches[0].token == 1, "match[0] token == 'a'");
        CHECK(matches[1].token == 2, "match[1] token == 'b'");
        CHECK(matches[2].token == 3, "match[2] token == 'c'");
    }
    free(matches);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: multi-byte tokens — longer match preferred over shorter
 * ========================================================================= */

static void test_longer_wins(void) {
    printf("\n[test_longer_wins]\n");
    /* "ab" should win over "a" for the input "ab" */
    const char *pats[] = { "a", "ab", "b" };
    const uint32_t ids[] = { 10, 20, 30 };
    AcAutomaton *a = build(pats, ids, 3);
    CHECK(a != NULL, "automaton built");

    const uint8_t text[] = "ab";
    size_t n;
    AcMatch *m = ac_leftmost_longest(a, text, 2, &n);
    CHECK(n == 1, "one match (longer wins)");
    if (n >= 1) CHECK(m[0].token == 20, "matched 'ab' (token 20)");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: no match returns empty
 * ========================================================================= */

static void test_no_match(void) {
    printf("\n[test_no_match]\n");
    const char *pats[] = { "xyz" };
    const uint32_t ids[] = { 99 };
    AcAutomaton *a = build(pats, ids, 1);

    const uint8_t text[] = "abc";
    size_t n;
    AcMatch *m = ac_leftmost_longest(a, text, 3, &n);
    CHECK(n == 0, "zero matches when no pattern found");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: overlapping iterator finds all occurrences of a substring
 * ========================================================================= */

static void test_overlapping_iter(void) {
    printf("\n[test_overlapping_iter]\n");
    /* Pattern "a" occurs at offsets 0,2,4 in "ababa" */
    const char *pats[] = { "a" };
    const uint32_t ids[] = { 1 };
    AcAutomaton *a = build(pats, ids, 1);

    const uint8_t text[] = "ababa";
    AcMatchIter *it = ac_iter_new(a, text, 5);
    CHECK(it != NULL, "iterator created");

    int found = 0;
    AcMatch m;
    while (ac_iter_next(it, &m)) found++;
    CHECK(found == 3, "iterator finds 3 occurrences of 'a'");
    ac_iter_free(it);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: empty string input
 * ========================================================================= */

static void test_empty_input(void) {
    printf("\n[test_empty_input]\n");
    const char *pats[] = { "a" };
    const uint32_t ids[] = { 7 };
    AcAutomaton *a = build(pats, ids, 1);

    size_t n;
    AcMatch *m = ac_leftmost_longest(a, (const uint8_t*)"", 0, &n);
    CHECK(n == 0, "no matches on empty input");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: patterns with shared prefixes (trie compression check)
 * ========================================================================= */

static void test_shared_prefix(void) {
    printf("\n[test_shared_prefix]\n");
    const char *pats[] = { "he", "her", "here", "hello" };
    const uint32_t ids[] = { 1, 2, 3, 4 };
    AcAutomaton *a = build(pats, ids, 4);
    CHECK(a != NULL, "automaton with shared prefixes built");

    /* "here" in text "here" — should match token 3 ("here") */
    const uint8_t text[] = "here";
    size_t n;
    AcMatch *m = ac_leftmost_longest(a, text, 4, &n);
    CHECK(n == 1, "one leftmost-longest match");
    if (n >= 1) CHECK(m[0].token == 3, "'here' matched (token 3)");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: binary / high-byte tokens
 * ========================================================================= */

static void test_binary_tokens(void) {
    printf("\n[test_binary_tokens]\n");
    const uint8_t pat[] = { 0xC3, 0xA9 }; /* UTF-8 for U+00E9 'é' */
    AcBuilder *b = ac_builder_new();
    ac_builder_add(b, pat, 2, 42);
    AcAutomaton *a = ac_builder_build(b);
    ac_builder_free(b);
    CHECK(a != NULL, "binary token automaton built");

    const uint8_t text[] = { 0xC3, 0xA9, 0xC3, 0xA9 };
    size_t n;
    AcMatch *m = ac_leftmost_longest(a, text, 4, &n);
    CHECK(n == 2, "two occurrences of 0xC3 0xA9");
    if (n >= 1) CHECK(m[0].token == 42, "first match token == 42");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Test: reverse iterator (for PrependableEncoder)
 * ========================================================================= */

static void test_reverse_iter(void) {
    printf("\n[test_reverse_iter]\n");
    /* Build reversed patterns: "ba", "a" (searching "abba" backwards reads as
     * the characters in input reversed, so pattern "ab" reversed is "ba") */
    const char *pats[] = { "ba", "a" };
    const uint32_t ids[] = { 10, 20 };
    AcAutomaton *a = build(pats, ids, 2);
    CHECK(a != NULL, "reverse automaton built");

    /* Walking backwards over "ab ba" reversed → "ab ba" reversed bytes */
    const uint8_t text[] = "abba"; /* reversed in mind: look for "ab" at end */
    size_t n;
    AcMatch *m = ac_leftmost_longest(a, text, 4, &n);
    /* leftmost-longest forward over "abba": 
       pos 0: 'a' matches token 20 (len 1), but "ab" (ba reversed) not found; 
       "ba" starts at pos 1 in "abba" — matches token 10 */
    CHECK(n > 0, "found at least one match in reverse test input");
    free(m);
    ac_automaton_free(a);
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(void) {
    printf("=== ac_bpe unit tests ===\n");
    test_single_char();
    test_longer_wins();
    test_no_match();
    test_overlapping_iter();
    test_empty_input();
    test_shared_prefix();
    test_binary_tokens();
    test_reverse_iter();
    printf("\nResults: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
