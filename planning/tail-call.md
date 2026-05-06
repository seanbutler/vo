---
status: PENDING
priority: (COULD, LATER, HIGH, BROAD)
---

# Tail-call optimisation

- The interpreter currently uses the C++ call stack for recursion
- Deep recursion (e.g. `range(1, 10000)`) will stack overflow
- Options: trampoline in `call_callable`, or explicit continuation passing
- Intentionally deferred — only needed if large recursion depths become a use case
