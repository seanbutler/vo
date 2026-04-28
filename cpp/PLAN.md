# PLAN

## Future Plan: Visitor-Style Dispatch

1. Introduce AST visitor interfaces for expressions and statements.
2. Add `accept(...)` methods to all AST node types.
3. Move runtime type selection from `Interpreter::eval` / `Interpreter::exec` into visitor methods.
4. Keep existing eval/exec helpers as implementation targets for visitor calls during migration.
5. Migrate incrementally and keep behavior parity with current tests.
6. Remove dynamic-cast chains once all nodes dispatch through visitors.
7. Add a regression test pass focused on dispatch equivalence.

## Notes

- This is intentionally deferred and not part of the current implementation.
- Goal: clearer extension path and stronger compile-time coverage when adding AST nodes.
