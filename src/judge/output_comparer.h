#pragma once

#include <string>

class OutputComparer {
public:
    static bool matches(const std::string& actual,
                        const std::string& expected);

    static std::string trim_trailing_whitespace(const std::string& value);
};
