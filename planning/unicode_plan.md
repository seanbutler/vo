# Unicode identifier support — implementation plan

## Goal

Allow UTF-8 / extended-ASCII bytes in VO identifiers so emoji and icon glyphs
can be used as variable names (e.g. `🐺 = wolf_state`).

All changes are in `interp/src/lexer/lexer.cpp` unless noted.

---

## Phase 1 Update Lexer to Accept Unicode

### Phase 1 Step 1 — Branch

move to a new branch "feature/unicode/core-lexer-change"

### Phase 1 Step 2 — Core lexer change (minimal viable)

**Two lines changed:**

```cpp
// line 123 — start of identifier
if (std::isalpha(c) || c == '_' || (static_cast<unsigned char>(c) > 127))

// line 104 — continuation in read_word()
while (!at_end() && (std::isalnum(peek()) || peek() == '_'
                     || (static_cast<unsigned char>(peek()) > 127)))
```

High bytes are passed through as opaque `std::string` bytes. The interpreter
stores and compares identifiers byte-for-byte, so this is sufficient for
basic emoji use.

### Phase 1 Step 3 - Test Coverage

- write tests or extend the existing tests so that they work with ascii as well as unicode identifiers.


### Phase 1 Step 4 - Merge and Update Documentation

- update documentation and examples to include unicode identifiers in at least one case. 
- add a paragraph to the README to explain rationale 
- once tests are passed merge into main




---

## Phase 2 — Reject bidirectional control characters (Trojan Source)

**CVE-2021-42574**: bidi control chars make source look different from what
it does when rendered. Reject them in both identifiers and string literals.

Affected UTF-8 sequences (all start with `0xE2`):

| Codepoint | UTF-8       | Name                        |
|-----------|-------------|-----------------------------|
| U+202A    | E2 80 AA    | LEFT-TO-RIGHT EMBEDDING     |
| U+202B    | E2 80 AB    | RIGHT-TO-LEFT EMBEDDING     |
| U+202C    | E2 80 AC    | POP DIRECTIONAL FORMATTING  |
| U+202D    | E2 80 AD    | LEFT-TO-RIGHT OVERRIDE      |
| U+202E    | E2 80 AE    | RIGHT-TO-LEFT OVERRIDE      |
| U+2066    | E2 81 A6    | LEFT-TO-RIGHT ISOLATE       |
| U+2067    | E2 81 A7    | RIGHT-TO-LEFT ISOLATE       |
| U+2068    | E2 81 A8    | FIRST STRONG ISOLATE        |
| U+2069    | E2 81 A9    | POP DIRECTIONAL ISOLATE     |

Detect these during tokenization (in `read_word` and `read_string`) and
throw a `LexError` naming the character.

---

## Phase 3 - Reject zero-width characters in identifiers

Zero-width chars embedded in identifiers make two visually identical names
actually distinct — a silent, hard-to-debug bug vector.

Affected codepoints:

| Codepoint | UTF-8       | Name                        |
|-----------|-------------|-----------------------------|
| U+200B    | E2 80 8B    | ZERO WIDTH SPACE            |
| U+200C    | E2 80 8C    | ZERO WIDTH NON-JOINER       |
| U+200D    | E2 80 8D    | ZERO WIDTH JOINER           |
| U+FEFF    | EF BB BF    | BOM / ZERO WIDTH NO-BREAK   |

Detect in `read_word()` continuation loop; throw `LexError` with the name.

---

## Phase 4 — Document homoglyph risk (no code change)

Visually similar characters from different Unicode blocks (e.g. Cyrillic `а`
U+0430 vs Latin `a` U+0061) are treated as **distinct** identifiers because
the lexer compares bytes, not glyphs.

This is intentional and matches how most languages behave without a
confusable-detection library. Add a comment near `read_word()` noting this
so future maintainers know it is a deliberate choice, not an oversight.

Full confusable detection (Unicode TR#39) requires a large data table and is
not worth the complexity for VO.

- Perhaps we make the table external and add a flag to switch detection on that outputs a warning?

---

## Phase 5 — NFC normalization of identifiers at lex time

Without normalization, decomposed and precomposed forms of the same character
(e.g. `e` + combining accent U+0301 vs precomposed `é` U+00E9) produce
different identifier strings that look identical.

Apply NFC normalization to the `buf` string at the end of `read_word()`.

**Dependency**: requires a Unicode normalization library.
- [`utfcpp`](https://github.com/nemtrif/utfcpp) — header-only, no NFC
- Small self-contained NFC table (a few hundred lines) — preferred
- ICU — overkill

**Defer until combining-character identifiers are actually used in practice.**

- Perhaps we make an external table and add a flag to switch detection on that outputs a warning?


---

## Phase 6 — Fix column counter for multi-byte characters

`advance()` currently increments `col_` by 1 per byte, so error positions
like `L5:3` are wrong (byte offset, not visual column) whenever multi-byte
characters appear earlier on the line.

Fix: only increment `col_` on leading bytes; skip continuation bytes.

```cpp
char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_; col_ = 1;
    } else if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
        // leading byte of a UTF-8 sequence (or ASCII) — counts as one column
        ++col_;
    }
    // continuation bytes (10xxxxxx) do not advance the column
    return c;
}
```

---

## Priority order

1. **Phase 1** — enables the feature (required)
2. **Phase 6** — cheap, makes error messages correct immediately
3. **Phase 2** — security hygiene, straightforward
4. **Phase 3** — prevents subtle bugs, straightforward
5. **Phase 4** — comment only, zero cost
6. **Phase 5** — defer; only matters if combining characters appear in practice
