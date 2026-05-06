---
status: PENDING
priority: (MUST, LATER)
---

# Output / stdio design

Current `printf_s`/`printf_i` are problematic. Type already encoded in the name, format string adds no value. Three options to choose from:

1. **Simple:** bind `puts(str)` for strings + a thin C wrapper `print_int(int)` — callers never see a format string
2. **Better:** full varargs `printf(fmt, ...)` FFI support — useful when padding/alignment/precision matter
3. **Current:** `printf_i("%d\n", n)` style — neither simple nor powerful. Fix this.
