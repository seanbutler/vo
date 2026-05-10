---
status: PENDING
priority: (MUST, SOONER, HIGH, BROAD)
depends: unicode_plan.md (phases 2–6 are Layer 1 of this plan)
---

# Localisation — three-layer architecture

VO has no reserved words and symbol-only syntax, making it uniquely suited to full natural-language localisation. This is also a general customisation mechanism and a way to program and build abstractions.

The goal is to separate three concerns cleanly:


1. Source (community symbols) -> symbol table -> canonical AST
2. Canonical AST -> interpreter
3. Interpreter errors -> error template table -> community-language diagnostics


## Layer 1 — UTF-8 support (lexer + runtime)

- Lexer currently treats source as raw bytes; multi-byte sequences may be mishandled
- Identifiers may contain UTF-8 characters (non-ASCII variable/function names)
- String literals preserve UTF-8 content correctly
- Line/column tracking counts Unicode codepoints, not bytes
- Operator glyphs in the symbol table may themselves be multi-byte
- Approach: decode source to codepoints before lexing, or UTF-8-aware character classification

See also `unicode_plan.md` for Phase 1 (done) and Phases 2–6 (pending).

## Layer 2 — Error message templates (runtime)

- Externalise all `RuntimeError` and `ParseError` strings into a template table
- Templates must support:
  - Argument reordering (Arabic, Japanese, Turkish word order)
  - Gender agreement metadata
  - Plural forms (Russian 4 forms, Arabic 6 forms)
  - RTL/LTR direction hints
- Consider Mustache-style templates for community accessibility
- Library options: mstch, kainjow/mustache, or std::format (C++20)
