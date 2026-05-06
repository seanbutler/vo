---
status: PENDING
priority: (MUST, LATER, LOW, BROAD)
---

# Enforce immutability at runtime

- `DeclStmt` already carries `is_mutable` flag but the interpreter does not enforce it
- `set()` on an immutable binding should throw `RuntimeError`
- Requires tracking mutability in `Environment` alongside the value
