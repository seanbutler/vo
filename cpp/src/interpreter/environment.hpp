#pragma once
#include "interpreter/value.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace lang {

// ── Environment ───────────────────────────────────────────────────────────────
//
//  A lexical scope frame.  Each frame has an optional parent, forming the
//  familiar scope chain used for variable lookup and closure capture.

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    // Define a new binding in THIS frame.
    void define(const std::string& name, ValuePtr value);

    // Look up a name, walking the parent chain.
    ValuePtr get(const std::string& name) const;

    // Assign to an existing binding (walks the chain to find it).
    void set(const std::string& name, ValuePtr value);

    // True if THIS frame (not parents) contains the name.
    bool has_local(const std::string& name) const;

    // Access the raw bindings (used by the import-* mechanism).
    const std::unordered_map<std::string, ValuePtr>& bindings() const;

private:
    std::unordered_map<std::string, ValuePtr> bindings_;
    std::shared_ptr<Environment>              parent_;
};

} // namespace lang
