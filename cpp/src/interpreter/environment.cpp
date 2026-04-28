#include "interpreter/environment.hpp"
#include <stdexcept>

namespace lang {

Environment::Environment(std::shared_ptr<Environment> parent)
    : parent_(std::move(parent)) {}

void Environment::define(const std::string& name, ValuePtr value) {
    bindings_[name] = std::move(value);
}

ValuePtr Environment::get(const std::string& name) const {
    auto it = bindings_.find(name);
    if (it != bindings_.end()) return it->second;
    if (parent_)               return parent_->get(name);
    throw std::runtime_error("Undefined variable '" + name + "'");
}

void Environment::set(const std::string& name, ValuePtr value) {
    auto it = bindings_.find(name);
    if (it != bindings_.end()) { it->second = std::move(value); return; }
    if (parent_)               { parent_->set(name, std::move(value)); return; }
    throw std::runtime_error("Undefined variable '" + name + "'");
}

bool Environment::has_local(const std::string& name) const {
    return bindings_.count(name) > 0;
}

const std::unordered_map<std::string, ValuePtr>& Environment::bindings() const {
    return bindings_;
}

} // namespace lang
