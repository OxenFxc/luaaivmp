#include "../Value.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

void test_is_functions() {
    Value nil_val = Nil{};
    Value bool_val = true;
    Value num_val = 42.0;
    Value str_val = std::string("hello");

    assert(is_nil(nil_val));
    assert(!is_nil(bool_val));
    assert(!is_nil(num_val));
    assert(!is_nil(str_val));

    assert(!is_boolean(nil_val));
    assert(is_boolean(bool_val));
    assert(!is_boolean(num_val));
    assert(!is_boolean(str_val));

    assert(!is_number(nil_val));
    assert(!is_number(bool_val));
    assert(is_number(num_val));
    assert(!is_number(str_val));

    assert(!is_string(nil_val));
    assert(!is_string(bool_val));
    assert(!is_string(num_val));
    assert(is_string(str_val));

    std::cout << "test_is_functions passed" << std::endl;
}

void test_as_boolean() {
    assert(as_boolean(Nil{}) == false);
    assert(as_boolean(true) == true);
    assert(as_boolean(false) == false);
    assert(as_boolean(42.0) == true);
    assert(as_boolean(0.0) == true);
    assert(as_boolean(std::string("hello")) == true);
    assert(as_boolean(std::string("")) == true);

    std::cout << "test_as_boolean passed" << std::endl;
}

void test_as_number() {
    assert(as_number(42.0) == 42.0);

    try {
        as_number(Nil{});
        assert(false && "Should have thrown std::runtime_error");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()) == "Value is not a number");
    }

    try {
        as_number(true);
        assert(false && "Should have thrown std::runtime_error");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()) == "Value is not a number");
    }

    std::cout << "test_as_number passed" << std::endl;
}

void test_as_string() {
    assert(as_string(std::string("hello")) == "hello");
    assert(as_string(42.0) == std::to_string(42.0));
    assert(as_string(true) == "true");
    assert(as_string(false) == "false");
    assert(as_string(Nil{}) == "nil");

    std::cout << "test_as_string passed" << std::endl;
}

int main() {
    test_is_functions();
    test_as_boolean();
    test_as_number();
    test_as_string();
    std::cout << "All Value tests passed!" << std::endl;
    return 0;
}
