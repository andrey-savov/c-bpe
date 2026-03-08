/* pretok.h — Hand-coded pretokenizer for cl100k and o200k.
 *
 * Replaces the PCRE2 regex-based pretokenizer with direct C code that
 * implements the same pattern matching using PCRE2's UCD tables for
 * Unicode property classification.
 */
#ifndef PRETOK_H
#define PRETOK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Match result
 * ========================================================================= */

typedef struct {
    size_t start;       /* byte offset of match start (relative to input) */
    size_t end;         /* byte offset of match end (relative to input) */
    int    pattern_id;  /* which pattern matched (0, 1, 2) */
    bool   lookahead;   /* strip last char from match? */
} PretokMatch;

/* =========================================================================
 * Pretokenizer function type
 *
 * A pretokenizer is a function pointer:
 *   bool pretok_fn(text, text_len, offset, is_tail, &match)
 *
 * Parameters:
 *   text      - full input text
 *   text_len  - total length in bytes
 *   offset    - byte position to match at (anchored)
 *   is_tail   - true if offset..text_len is the remainder of the original input
 *               (needed for \s+$ semantics)
 *   match     - output match result
 *
 * Returns true if a match was found, false otherwise.
 * ========================================================================= */

typedef bool (*pretok_fn)(const uint8_t *text, size_t text_len,
                          size_t offset, PretokMatch *match);

/* cl100k pretokenizer */
bool pretok_cl100k(const uint8_t *text, size_t text_len,
                   size_t offset, PretokMatch *match);

/* o200k pretokenizer */
bool pretok_o200k(const uint8_t *text, size_t text_len,
                  size_t offset, PretokMatch *match);

/* =========================================================================
 * UTF-8 decode helper (shared)
 * ========================================================================= */

/* Decode one UTF-8 codepoint starting at p. Returns codepoint and sets
 * *len to the number of bytes consumed (1-4). On invalid UTF-8, returns
 * the byte value as-is and sets *len to 1. */
static inline uint32_t utf8_decode(const uint8_t *p, const uint8_t *end,
                                   int *len) {
    uint8_t b0 = *p;
    if (b0 < 0x80) {
        *len = 1;
        return b0;
    }
    if ((b0 & 0xE0) == 0xC0 && p + 1 < end && (p[1] & 0xC0) == 0x80) {
        *len = 2;
        return ((uint32_t)(b0 & 0x1F) << 6) | (p[1] & 0x3F);
    }
    if ((b0 & 0xF0) == 0xE0 && p + 2 < end &&
        (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *len = 3;
        return ((uint32_t)(b0 & 0x0F) << 12) |
               ((uint32_t)(p[1] & 0x3F) << 6) |
               (p[2] & 0x3F);
    }
    if ((b0 & 0xF8) == 0xF0 && p + 3 < end &&
        (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 &&
        (p[3] & 0xC0) == 0x80) {
        *len = 4;
        return ((uint32_t)(b0 & 0x07) << 18) |
               ((uint32_t)(p[1] & 0x3F) << 12) |
               ((uint32_t)(p[2] & 0x3F) << 6) |
               (p[3] & 0x3F);
    }
    /* Invalid UTF-8: consume one byte */
    *len = 1;
    return b0;
}

/* =========================================================================
 * Unicode property helpers using PCRE2's UCD tables
 * ========================================================================= */

/* We include pcre2_internal.h indirectly via the UCD externs.
 * These helpers are defined in pretok_ucd.h which is included by the .c files. */

#ifdef __cplusplus
}
#endif
#endif /* PRETOK_H */
