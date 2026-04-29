#include "interpreter/value.hpp"
#include <sstream>
#include <stdexcept>

namespace lang {

// ── HashInstance ──────────────────────────────────────────────────────────────

HashPtr HashInstance::clone() const {
    auto h = std::make_shared<HashInstance>();
    h->members = members;   // shallow copy — each instance owns its member map
    return h;
}

ValuePtr HashInstance::get(const std::string& name) const {
    auto it = members.find(name);
    if (it == members.end())
        throw std::runtime_error("Hash has no member '" + name + "'");
    return it->second;
}

void HashInstance::set(const std::string& name, ValuePtr v) {
    members[name] = std::move(v);
}

bool HashInstance::has(const std::string& name) const {
    return members.count(name) > 0;
}

// ── Value ─────────────────────────────────────────────────────────────────────

bool Value::is_truthy() const {
    switch (kind()) {
    case Kind::Nil:      return false;
    case Kind::Integer:  return as_int() != 0;
    case Kind::Double:   return as_double() != 0.0;
    case Kind::String:   return !as_string().empty();
    case Kind::Hash:     return true;   // hashes are always truthy (use is_empty member pattern)
    case Kind::Callable: return true;
    case Kind::NativeCallable: return true;
    }
    return false;
}

std::string Value::to_display_string() const {
    switch (kind()) {
    case Kind::Nil:      return "{}";
    case Kind::Integer:  return std::to_string(as_int());
    case Kind::Double: {
        std::ostringstream oss;
        oss << as_double();
        return oss.str();
    }
    case Kind::String:   return as_string();
    case Kind::Callable: return "<callable>";
    case Kind::NativeCallable: return "<native:" + as_native_callable()->name + ">";
    case Kind::Hash: {
        std::ostringstream oss;
        oss << "{ ";
        bool first = true;
        for (auto& [k, v] : as_hash()->members) {
            if (k == "()") continue;    // skip constructor slot in display
            if (!first) oss << ", ";
            oss << k << " = " << v->to_display_string();
            first = false;
        }
        oss << " }";
        return oss.str();
    }
    }
    return "?";
}

} // namespace lang
