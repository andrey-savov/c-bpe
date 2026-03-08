/* pretok_cl100k.c — Hand-coded pretokenizer for cl100k_base.
 *
 * Implements the same 3-pattern alternation as the PCRE2 version:
 *
 *   Pattern 0 (no lookahead):
 *     (?i:'s|'t|'re|'ve|'m|'ll|'d)
 *     | [^\r\n\p{L}\p{N}]?\p{L}+
 *     | \p{N}{1,3}
 *     | ' '?[^\s\p{L}\p{N}]+[\r\n]*
 *     | \s*[\r\n]+
 *     | \s+$
 *
 *   Pattern 1 (lookahead=true):  \s+\s
 *   Pattern 2 (no lookahead):    \s+
 *
 * The 3 patterns are tried in order. Pattern 0 has sub-alternatives tried
 * left-to-right (first match wins). Matches are anchored at the current
 * position.
 */
#include <string.h>
#include "pretok.h"
#include "pretok_ucd.h"

/* =========================================================================
 * Local helpers
 * ========================================================================= */

/* Peek at the codepoint at p without advancing. */
static inline uint32_t peek_cp(const uint8_t *p, const uint8_t *end, int *len) {
    if (p >= end) { *len = 0; return 0; }
    return utf8_decode(p, end, len);
}

/* Is byte one of \r \n? */
static inline bool is_crlf_byte(uint8_t b) {
    return b == '\r' || b == '\n';
}

/* Try to match a contraction starting with apostrophe at pos.
 * (?i:'s|'t|'re|'ve|'m|'ll|'d)
 * Returns number of bytes matched (2 or 3), or 0. */
static size_t try_contraction(const uint8_t *p, const uint8_t *end) {
    size_t avail = (size_t)(end - p);
    if (avail < 2) return 0;
    /* p[0] must be ' (apostrophe, 0x27) or \u2019 (right single quote, E2 80 99) */
    uint8_t b0 = p[0];
    const uint8_t *after_apos;
    if (b0 == '\'') {
        after_apos = p + 1;
    } else if (b0 == 0xE2 && avail >= 3 && p[1] == 0x80 && p[2] == 0x99) {
        /* \u2019 RIGHT SINGLE QUOTATION MARK */
        after_apos = p + 3;
    } else {
        return 0;
    }
    if (after_apos >= end) return 0;

    size_t apos_len = (size_t)(after_apos - p);
    uint8_t c = *after_apos | 0x20; /* lowercase */

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

/* Sub 0a: (?i:'s|'t|'re|'ve|'m|'ll|'d) */
static bool p0_contraction(const uint8_t *text, size_t text_len,
                           size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    size_t n = try_contraction(p, end);
    if (n == 0) return false;
    m->start = offset;
    m->end = offset + n;
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0b: [^\r\n\p{L}\p{N}]?\p{L}+ */
static bool p0_letters(const uint8_t *text, size_t text_len,
                       size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    const uint8_t *start = p;
    int clen;

    /* Optional [^\r\n\p{L}\p{N}] */
    if (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (!ucd_is_newline(cp) && !ucd_is_letter(cp) && !ucd_is_number(cp)) {
            /* It's a valid prefix char — but only consume if followed by \p{L} */
            const uint8_t *after = p + clen;
            if (after < end) {
                int clen2;
                uint32_t cp2 = utf8_decode(after, end, &clen2);
                if (ucd_is_letter(cp2)) {
                    p = after; /* consume the optional prefix */
                }
            }
        }
    }

    /* \p{L}+ (one or more letters, greedy) */
    const uint8_t *letter_start = p;
    while (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (!ucd_is_letter(cp)) break;
        p += clen;
    }

    if (p == letter_start) return false; /* no letters consumed */

    m->start = offset;
    m->end = (size_t)(p - text);
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

    const uint8_t *start = p;
    while (p < end && count < 3) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (!ucd_is_number(cp)) break;
        p += clen;
        count++;
    }

    if (count == 0) return false;

    m->start = offset;
    m->end = (size_t)(p - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0d: ' '?[^\s\p{L}\p{N}]+[\r\n]* */
static bool p0_punct(const uint8_t *text, size_t text_len,
                     size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* Optional space (literal ' ') */
    if (p < end && *p == ' ') {
        /* Only consume if followed by [^\s\p{L}\p{N}] */
        if (p + 1 < end) {
            int clen2;
            uint32_t cp2 = utf8_decode(p + 1, end, &clen2);
            if (!ucd_is_whitespace(cp2) && !ucd_is_letter(cp2) && !ucd_is_number(cp2)) {
                p++; /* consume space */
            }
        }
    }

    /* [^\s\p{L}\p{N}]+ (one or more non-space non-letter non-digit, greedy) */
    const uint8_t *run_start = p;
    while (p < end) {
        uint32_t cp = utf8_decode(p, end, &clen);
        if (ucd_is_whitespace(cp) || ucd_is_letter(cp) || ucd_is_number(cp))
            break;
        p += clen;
    }

    if (p == run_start) return false; /* no chars consumed */

    /* [\r\n]* (greedy) */
    while (p < end && is_crlf_byte(*p))
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

    /* \s* (zero or more whitespace, greedy) — but we need at least [\r\n] after */
    const uint8_t *ws_end = p;
    while (ws_end < end) {
        uint32_t cp = utf8_decode(ws_end, end, &clen);
        if (!ucd_is_whitespace(cp)) break;
        ws_end += clen;
    }

    /* Now scan backwards from ws_end to find where [\r\n]+ starts.
     * Actually, simpler: skip \s* that are not \r\n, then require [\r\n]+. */
    /* Re-scan from p: skip non-newline whitespace, then require [\r\n]+. */
    const uint8_t *q = p;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (ucd_is_newline(cp)) break;  /* found newline */
        if (!ucd_is_whitespace(cp)) return false;  /* non-ws before newline */
        q += clen;
    }

    if (q >= end) return false; /* no newline found */

    /* [\r\n]+ (greedy) */
    while (q < end && is_crlf_byte(*q))
        q++;

    m->start = offset;
    m->end = (size_t)(q - text);
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* Sub 0f: \s+$ (trailing whitespace at end of full input) */
static bool p0_trailing_ws(const uint8_t *text, size_t text_len,
                           size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* \s+ greedy, must reach end of input */
    const uint8_t *q = p;
    while (q < end) {
        uint32_t cp = utf8_decode(q, end, &clen);
        if (!ucd_is_whitespace(cp)) return false; /* non-ws before end → fail */
        q += clen;
    }

    if (q == p) return false; /* empty match */

    m->start = offset;
    m->end = text_len;
    m->pattern_id = 0;
    m->lookahead = false;
    return true;
}

/* =========================================================================
 * Pattern 1: \s+\s (lookahead=true → match \s{2,}, strip last char)
 * ========================================================================= */

static bool p1_ws_lookahead(const uint8_t *text, size_t text_len,
                            size_t offset, PretokMatch *m) {
    const uint8_t *p = text + offset;
    const uint8_t *end = text + text_len;
    int clen;

    /* First char must be \s */
    if (p >= end) return false;
    uint32_t cp = utf8_decode(p, end, &clen);
    if (!ucd_is_whitespace(cp)) return false;
    const uint8_t *q = p + clen;

    /* Consume \s+ greedily */
    while (q < end) {
        cp = utf8_decode(q, end, &clen);
        if (!ucd_is_whitespace(cp)) break;
        q += clen;
    }

    /* We need at least 2 whitespace chars (\s+\s means consume \s then \s) */
    /* Actually \s+\s means: \s+ followed by \s — so total >= 2 ws codepoints */
    /* And with lookahead, we strip the last one */
    size_t match_end = (size_t)(q - text);
    if (match_end - offset <= (size_t)clen && q == p + clen) {
        /* Only 1 ws char — need at least 2 for \s+\s */
        return false;
    }

    /* Recount: we need the match to be \s+\s which is >=2 ws chars.
     * The greedy match consumed all ws. We have at least 2 if q > p + first_clen. */
    /* Check: was there more than one ws codepoint? */
    int first_len;
    utf8_decode(p, end, &first_len);
    if (q <= p + first_len) return false; /* only 1 ws char */

    m->start = offset;
    m->end = match_end;
    m->pattern_id = 1;
    m->lookahead = true;  /* strip last ws char */
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
 * Top-level cl100k pretokenizer
 *
 * Tries patterns in order: 0 (sub-alternatives a-f), 1, 2.
 * First match wins (leftmost-first alternation).
 * ========================================================================= */

bool pretok_cl100k(const uint8_t *text, size_t text_len,
                   size_t offset, PretokMatch *match) {
    if (offset >= text_len) return false;

    /* Pattern 0: try each sub-alternative in order */
    if (p0_contraction(text, text_len, offset, match)) return true;
    if (p0_letters(text, text_len, offset, match)) return true;
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
