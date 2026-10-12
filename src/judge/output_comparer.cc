#include "output_comparer.h"

std::string OutputComparer::trim_trailing_whitespace(
    const std::string& value) {
    std::size_t end = value.size();
    while (end > 0) {
        const char current = value[end - 1];
        if (current != ' ' && current != '\t' &&
            current != '\r' && current != '\n' &&
            current != '\v' && current != '\f') {
            break;
        }
        --end;
    }
    return value.substr(0, end);
}

bool OutputComparer::matches(const std::string& actual,
                             const std::string& expected) {
    return trim_trailing_whitespace(actual) ==
           trim_trailing_whitespace(expected);
}
