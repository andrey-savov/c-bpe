/* pretok_o200k.c — Hand-coded pretokenizer for o200k_base.
 *
 * Implements 3-pattern alternation:
 *
 *   Pattern 0 (no lookahead):
 *     [^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]*[\p{Ll}\p{Lm}\p{Lo}\p{M}]+(?i:'s|'t|'re|'ve|'m|'ll|'d)?
 *     | [^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]+[\p{Ll}\p{Lm}\p{Lo}\p{M}]*(?i:'s|'t|'re|'ve|'m|'ll|'d)?
 *     | \p{N}{1,3}
 *     | ' '?[^\s\p{L}\p{N}]+[\r\n/]*
 *     | \s*[\r\n]+
 *     | \s+$
 *
 *   Pattern 1 (lookahead=true):  \s+\s
 *   Pattern 2 (no lookahead):    \s+
 *
 * The main difference from cl100k is the first two sub-alternatives which
 * distinguish CamelCase from ALLCAPS words using Unicode subcategories.
 */
#include <string.h>
#include "pretok.h"
#include "pretok_ucd.h"

/* =========================================================================
 * Local helpers
 * ========================================================================= */

static inline uint32_t peek_cp(const uint8_t *p, const uint8_t *end, int *len) {
    if (p >= end) { *len = 0; return 0; }
    return utf8_decode(p, end, len);
}

static inline bool is_crlf_byte(uint8_t b) {
    return b == '\r' || b == '\n';
}

/* Try to match a contraction at pos.
 * (?i:'s|'t|'re|'ve|'m|'ll|'d)
 * Returns number of bytes matched (2 or 3), or 0. */
static size_t try_contraction(const uint8_t *p, const uint8_t *end) {
    size_t avail = (size_t)(end - p);
    if (avail < 2) return 0;
    uint8_t b0 = p[0];
    const uint8_t *after_apos;
    if (b0 == '\'') {
        after_apos = p + 1;
    } else if (b0 == 0xE2 && avail >= 3 && p[1] == 0x80 && p[2] == 0x99) {
        after_apos = p + 3;
    } else {
        return 0;
    }
    if (after_apos >= end) return 0;

    size_t apos_len = (size_t)(after_apos - p);
    uint8_t c = *after_apos | 0x20;

    switch (c) {
        case 's': case 't': case 'm': case 'd':
            return apos_len + 1;
        case 'r':
            if (after_apos + 1 < end && (after_apos[1] | 0x20) == 'e')
                return apos_len + 2;
            return 0;
        case 'v':
            if (after_apos + 1 < end && (after_apos[1] | 0x20) == 'e')
                return apos_len + 2;
            return 0;
        case 'l':
            if (after_apos + 1 < end && (after_apos[1] | 0x20) == 'l')
                return apos_len + 2;
            return 0;
        default:
            return 0;
    }
}

/* =========================================================================
 * Pattern 0 sub-alternatives
 * ========================================================================= */

/* Sub 0a: [^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]*[\p{Ll}\p{Lm}\p{Lo}\p{M}]+(?i:'s|...)?
 * CamelCase word: optional non-L-N prefix, optional upper-class chars,
 * then required lower-class chars, then optional contraction. */
static bool p0_camel(const uint8_t *text, size_t text_len,
                     size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* Optional [^\r\n\p{L}\p{N}] */
    const uint8_t *after_prefix = p;
    if (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (!ucd_is_newline(cp) && !ucd_is_letter(cp) && !ucd_is_number(cp)) {
            after_prefix = p + clen;
        }
    }

    /* [\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]* (zero or more upper-class) */
    const uint8_t *q = after_prefix;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_upper_class(cp)) break;
        q += clen;
    }

    /* [\p{Ll}\p{Lm}\p{Lo}\p{M}]+ (one or more lower-class, required) */
    const uint8_t *lower_start = q;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_lower_class(cp)) break;
        q += clen;
    }
    if (q == lower_start) return false; /* no lower-class chars */

    /* Verify we consumed something beyond the prefix */
    if (q == p) return false;

    /* Only consume the prefix if we actually had upper/lower chars after it */
    if (after_prefix > p && after_prefix == lower_start && q == lower_start)
        return false;

    /* Optional contraction (?i:'s|'t|'re|'ve|'m|'ll|'d)? */
    size_t contr = try_contraction(q, end);
    q += contr;

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0b: [^\r\n\p{L}\p{N}]?[\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]+[\p{Ll}\p{Lm}\p{Lo}\p{M}]*(?i:'s|...)?
 * ALLCAPS word: optional prefix, required upper-class chars,
 * optional lower-class chars, optional contraction. */
static bool p0_allcaps(const uint8_t *text, size_t text_len,
                       size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* Optional [^\r\n\p{L}\p{N}] */
    const uint8_t *after_prefix = p;
    if (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (!ucd_is_newline(cp) && !ucd_is_letter(cp) && !ucd_is_number(cp)) {
            after_prefix = p + clen;
        }
    }

    /* [\p{Lu}\p{Lt}\p{Lm}\p{Lo}\p{M}]+ (one or more upper-class, required) */
    const uint8_t *q = after_prefix;
    const uint8_t *upper_start = q;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_upper_class(cp)) break;
        q += clen;
    }
    if (q == upper_start) return false; /* no upper-class chars */

    /* Verify we consumed something beyond the prefix */
    if (q == p) return false;

    /* Only consume the prefix if we actually had upper chars after it */
    if (after_prefix > p && upper_start == after_prefix && q == upper_start)
        return false;

    /* [\p{Ll}\p{Lm}\p{Lo}\p{M}]* (zero or more lower-class) */
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_lower_class(cp)) break;
        q += clen;
    }

    /* Optional contraction */
    size_t contr = try_contraction(q, end);
    q += contr;

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0c: \p{N}{1,3} */
static bool p0_numbers(const uint8_t *text, size_t text_len,
                       size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;
    int count = 0;

    const uint8_t *q = p;
    while (q < end && count < 3) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_number(cp)) break;
        q += clen;
        count++;
    }

    if (count == 0) return false;

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0d: ' '?[^\s\p{L}\p{N}]+[\r\n/]* — note [\r\n/]* for o200k */
static bool p0_punct(const uint8_t *text, size_t text_len,
                     size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* Optional space */
    if (p < end && *p == ' ') {
        if (p + 1 < end) {
            int clen2;
            uint32_t cp2 = utf8_decode(p + 1, end, &clen2);
            if (!ucd_is_whitespace(cp2) && !ucd_is_letter(cp2) && !ucd_is_number(cp2)) {
                p++;
            }
        }
    }

    /* [^\s\p{L}\p{N}]+ */
    const uint8_t *run_start = p;
    while (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (ucd_is_whitespace(cp) || ucd_is_letter(cp) || ucd_is_number(cp))
            break;
        p += clen;
    }

    if (p == run_start) return false;

    /* [\r\n/]* — o200k includes / */
    while (p < end && (is_crlf_byte(*p) || *p == '/'))
        p++;

    m->start = offset;
    m->end = (size_t)(p - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0e: \s*[\r\n]+ */
static bool p0_ws_newlines(const uint8_t *text, size_t text_len,
                           size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* Skip non-newline whitespace */
    const uint8_t *q = p;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (ucd_is_newline(cp)) break;
        if (!ucd_is_whitespace(cp)) return false;
        q += clen;
    }

    if (q >= end) return false;

    /* [\r\n]+ */
    while (q < end && is_crlf_byte(*q))
        q++;

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0f: \s+$ */
static bool p0_trailing_ws(const uint8_t *text, size_t text_len,
                           size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    const uint8_t *q = p;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_whitespace(cp)) return false;
        q += clen;
    }

    if (q == p) return false;

    m->start = offset;
    m->end = text_len;
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* =========================================================================
 * Pattern 1: \s+\s (lookahead=true)
 * ========================================================================= */

static bool p1_ws_lookahead(const uint8_t *text, size_t text_len,
                            size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    if (p >= end) return false;
    uint32_t cp = utf8_decode(p, end, &clen);
    if (!ucd_is_whitespace(cp)) return false;
    int first_len = clen;
    const uint8_t *q = p + clen;

    while (q < end) {
        cp = utf8_decode(q, end, &clen);
        if (!ucd_is_whitespace(cp)) break;
        q += clen;
    }

    if (q <= p + first_len) return false; /* only 1 ws char */

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 1;
    m->lookahead = true;
    return true;
}

/* =========================================================================
 * Pattern 2: \s+
 * ========================================================================= */

static bool p2_ws(const uint8_t *text, size_t text_len,
                  size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    if (p >= end) return false;
    uint32_t cp = utf8_decode(p, end, &clen);
    if (!ucd_is_whitespace(cp)) return false;
    const uint8_t *q = p + clen;

    while (q < end) {
        cp = utf8_decode(q, end, &clen);
        if (!ucd_is_whitespace(cp)) break;
        q += clen;
    }

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 2;
    m->lookahead = false;
    return true;
}

/* =========================================================================
 * Top-level o200k pretokenizer
 * ========================================================================= */

bool pretok_o200k(const uint8_t *text, size_t text_len,
                  size_t offset, PretokMatch *match) {
    if (offset >= text_len) return false;

    /* Pattern 0: try each sub-alternative in order */
    if (p0_camel(text, text_len, offset, match)) return true;
    if (p0_allcaps(text, text_len, offset, match)) return true;
    if (p0_numbers(text, text_len, offset, match)) return true;
    if (p0_punct(text, text_len, offset, match)) return true;
    if (p0_ws_newlines(text, text_len, offset, match)) return true;
    if (p0_trailing_ws(text, text_len, offset, match)) return true;

    /* Pattern 1: \s+\s (lookahead) */
    if (p1_ws_lookahead(text, text_len, offset, match)) return true;

    /* Pattern 2: \s+ */
    if (p2_ws(text, text_len, offset, match)) return true;

    return false;
}
