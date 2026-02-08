#ifndef VALUE_H
#define VALUE_H

#include <variant>
#include <string>
#include <iostream>

struct Nil {};

using Value = std::variant<Nil, bool, double, std::string>;

inline bool is_nil(const Value& v) {
    return std::holds_alternative<Nil>(v);
}

inline bool is_boolean(const Value& v) {
    return std::holds_alternative<bool>(v);
}

inline bool is_number(const Value& v) {
    return std::holds_alternative<double>(v);
}

inline bool is_string(const Value& v) {
    return std::holds_alternative<std::string>(v);
}

inline bool as_boolean(const Value& v) {
    if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v);
    }
    return !std::holds_alternative<Nil>(v); // Lua truthiness: nil is false, numbers are true
}

inline double as_number(const Value& v) {
    if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    }
    throw std::runtime_error("Value is not a number");
}

inline std::string as_string(const Value& v) {
    if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    }
    if (std::holds_alternative<double>(v)) {
        return std::to_string(std::get<double>(v));
    }
    if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    }
    return "nil";
}

#endif
