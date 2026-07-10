#pragma once

#include "doof_runtime.hpp"

namespace doof_json_detail {

inline bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

inline int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
}

inline void append_codepoint_utf8(std::string& out, uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
        return;
    }
    if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        return;
    }
    if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        return;
    }
    out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
}

struct Parser {
    const std::string& text;
    size_t index = 0;

    [[nodiscard]] bool at_end() const {
        return index >= text.size();
    }

    [[nodiscard]] char peek() const {
        return at_end() ? '\0' : text[index];
    }

    void skip_whitespace() {
        while (!at_end() && std::isspace(static_cast<unsigned char>(text[index]))) {
            ++index;
        }
    }

    [[nodiscard]] std::string error(const std::string& message) const {
        size_t line = 1;
        size_t column = 1;
        for (size_t cursor = 0; cursor < index && cursor < text.size(); ++cursor) {
            if (text[cursor] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        return message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    }

    doof::Result<doof::JsonValue, std::string> parse_document() {
        skip_whitespace();
        auto value = parse_value();
        if (is_failure(value)) {
            return value;
        }
        skip_whitespace();
        if (!at_end()) {
            return doof::Failure<std::string>{error("Unexpected trailing characters")};
        }
        return value;
    }

    doof::Result<doof::JsonValue, std::string> parse_value() {
        if (at_end()) {
            return doof::Failure<std::string>{error("Unexpected end of JSON input")};
        }
        switch (peek()) {
            case 'n': return parse_null();
            case 't': return parse_true();
            case 'f': return parse_false();
            case '"': {
                auto parsed = parse_string();
                if (is_failure(parsed)) {
                    return doof::Failure<std::string>{failure_error(parsed)};
                }
                return doof::Success<doof::JsonValue>{doof::JsonValue(std::move(success_value(parsed)))};
            }
            case '[': return parse_array();
            case '{': return parse_object();
            default:
                if (peek() == '-' || is_digit(peek())) {
                    return parse_number();
                }
                return doof::Failure<std::string>{error("Unexpected character in JSON input")};
        }
    }

    doof::Result<doof::JsonValue, std::string> parse_null() {
        if (text.compare(index, 4, "null") != 0) {
            return doof::Failure<std::string>{error("Invalid token")};
        }
        index += 4;
        return doof::Success<doof::JsonValue>{doof::JsonValue(nullptr)};
    }

    doof::Result<doof::JsonValue, std::string> parse_true() {
        if (text.compare(index, 4, "true") != 0) {
            return doof::Failure<std::string>{error("Invalid token")};
        }
        index += 4;
        return doof::Success<doof::JsonValue>{doof::JsonValue(true)};
    }

    doof::Result<doof::JsonValue, std::string> parse_false() {
        if (text.compare(index, 5, "false") != 0) {
            return doof::Failure<std::string>{error("Invalid token")};
        }
        index += 5;
        return doof::Success<doof::JsonValue>{doof::JsonValue(false)};
    }

    doof::Result<std::string, std::string> parse_string() {
        if (peek() != '"') {
            return doof::Failure<std::string>{error("Expected JSON string")};
        }
        ++index;
        std::string out;
        while (!at_end()) {
            const unsigned char ch = static_cast<unsigned char>(text[index++]);
            if (ch == '"') {
                return doof::Success<std::string>{std::move(out)};
            }
            if (ch == '\\') {
                if (at_end()) {
                    return doof::Failure<std::string>{error("Unterminated escape sequence")};
                }
                const char escape = text[index++];
                switch (escape) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        auto codepoint = parse_unicode_escape();
                        if (is_failure(codepoint)) {
                            return doof::Failure<std::string>{failure_error(codepoint)};
                        }
                        append_codepoint_utf8(out, success_value(codepoint));
                        break;
                    }
                    default:
                        return doof::Failure<std::string>{error("Invalid escape sequence")};
                }
                continue;
            }
            if (ch < 0x20) {
                return doof::Failure<std::string>{error("Unescaped control character in string")};
            }
            out.push_back(static_cast<char>(ch));
        }
        return doof::Failure<std::string>{error("Unterminated string literal")};
    }

    doof::Result<uint32_t, std::string> parse_unicode_escape() {
        uint32_t codepoint = 0;
        for (int i = 0; i < 4; ++i) {
            if (at_end()) {
                return doof::Failure<std::string>{error("Incomplete unicode escape")};
            }
            const int value = hex_value(text[index++]);
            if (value < 0) {
                return doof::Failure<std::string>{error("Invalid unicode escape")};
            }
            codepoint = (codepoint << 4) | static_cast<uint32_t>(value);
        }

        if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
            if (index + 1 >= text.size() || text[index] != '\\' || text[index + 1] != 'u') {
                return doof::Failure<std::string>{error("Expected unicode low surrogate")};
            }
            index += 2;
            uint32_t low = 0;
            for (int i = 0; i < 4; ++i) {
                if (at_end()) {
                    return doof::Failure<std::string>{error("Incomplete unicode escape")};
                }
                const int value = hex_value(text[index++]);
                if (value < 0) {
                    return doof::Failure<std::string>{error("Invalid unicode escape")};
                }
                low = (low << 4) | static_cast<uint32_t>(value);
            }
            if (low < 0xDC00 || low > 0xDFFF) {
                return doof::Failure<std::string>{error("Invalid unicode low surrogate")};
            }
            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
        } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
            return doof::Failure<std::string>{error("Unexpected unicode low surrogate")};
        }

        return doof::Success<uint32_t>{codepoint};
    }

    doof::Result<doof::JsonValue, std::string> parse_array() {
        ++index;
        skip_whitespace();
        auto result = std::make_shared<std::vector<doof::JsonValue>>();
        if (peek() == ']') {
            ++index;
            return doof::Success<doof::JsonValue>{doof::JsonValue(std::move(result))};
        }
        while (true) {
            auto item = parse_value();
            if (is_failure(item)) {
                return item;
            }
            result->push_back(std::move(success_value(item)));
            skip_whitespace();
            if (peek() == ']') {
                ++index;
                break;
            }
            if (peek() != ',') {
                return doof::Failure<std::string>{error("Expected ',' or ']'")};
            }
            ++index;
            skip_whitespace();
        }
        return doof::Success<doof::JsonValue>{doof::JsonValue(std::move(result))};
    }

    doof::Result<doof::JsonValue, std::string> parse_object() {
        ++index;
        skip_whitespace();
        doof::JsonObject result = std::make_shared<doof::JsonObject::element_type>();
        if (peek() == '}') {
            ++index;
            return doof::Success<doof::JsonValue>{doof::JsonValue(std::move(result))};
        }
        while (true) {
            auto key = parse_string();
            if (is_failure(key)) {
                return doof::Failure<std::string>{failure_error(key)};
            }
            skip_whitespace();
            if (peek() != ':') {
                return doof::Failure<std::string>{error("Expected ':' after object key")};
            }
            ++index;
            skip_whitespace();
            auto value = parse_value();
            if (is_failure(value)) {
                return value;
            }
            (*result)[std::move(success_value(key))] = std::move(success_value(value));
            skip_whitespace();
            if (peek() == '}') {
                ++index;
                break;
            }
            if (peek() != ',') {
                return doof::Failure<std::string>{error("Expected ',' or '}'")};
            }
            ++index;
            skip_whitespace();
        }
        return doof::Success<doof::JsonValue>{doof::JsonValue(std::move(result))};
    }

    doof::Result<doof::JsonValue, std::string> parse_number() {
        const size_t start = index;
        if (peek() == '-') {
            ++index;
            if (at_end()) {
                return doof::Failure<std::string>{error("Invalid JSON number")};
            }
        }

        if (peek() == '0') {
            ++index;
            if (!at_end() && is_digit(peek())) {
                return doof::Failure<std::string>{error("Leading zeros are not allowed in JSON numbers")};
            }
        } else {
            if (!is_digit(peek())) {
                return doof::Failure<std::string>{error("Invalid JSON number")};
            }
            while (!at_end() && is_digit(peek())) {
                ++index;
            }
        }

        bool is_float = false;
        if (!at_end() && peek() == '.') {
            is_float = true;
            ++index;
            if (at_end() || !is_digit(peek())) {
                return doof::Failure<std::string>{error("Invalid JSON number")};
            }
            while (!at_end() && is_digit(peek())) {
                ++index;
            }
        }

        if (!at_end() && (peek() == 'e' || peek() == 'E')) {
            is_float = true;
            ++index;
            if (!at_end() && (peek() == '+' || peek() == '-')) {
                ++index;
            }
            if (at_end() || !is_digit(peek())) {
                return doof::Failure<std::string>{error("Invalid JSON number exponent")};
            }
            while (!at_end() && is_digit(peek())) {
                ++index;
            }
        }

        const std::string token = text.substr(start, index - start);
        if (is_float) {
            return parse_float_number(token);
        }
        return parse_integer_number(token);
    }

    doof::Result<doof::JsonValue, std::string> parse_float_number(const std::string& token) {
        errno = 0;
        char* end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (end == token.c_str() || (end != nullptr && *end != 0)) {
            return doof::Failure<std::string>{error("Invalid JSON number")};
        }
        if (errno == ERANGE || !std::isfinite(value)) {
            return doof::Failure<std::string>{error("JSON number out of range")};
        }
        return doof::Success<doof::JsonValue>{doof::JsonValue(value)};
    }

    doof::Result<doof::JsonValue, std::string> parse_integer_number(const std::string& token) {
        if (!token.empty() && token.front() == '-') {
            errno = 0;
            char* end = nullptr;
            const long long value = std::strtoll(token.c_str(), &end, 10);
            if (end != nullptr && *end == 0 && errno != ERANGE) {
                if (value >= std::numeric_limits<int32_t>::min() && value <= std::numeric_limits<int32_t>::max()) {
                    return doof::Success<doof::JsonValue>{doof::JsonValue(static_cast<int32_t>(value))};
                }
                return doof::Success<doof::JsonValue>{doof::JsonValue(static_cast<int64_t>(value))};
            }
        } else {
            errno = 0;
            char* end = nullptr;
            const unsigned long long value = std::strtoull(token.c_str(), &end, 10);
            if (end != nullptr && *end == 0 && errno != ERANGE) {
                if (value <= static_cast<unsigned long long>(std::numeric_limits<int32_t>::max())) {
                    return doof::Success<doof::JsonValue>{doof::JsonValue(static_cast<int32_t>(value))};
                }
                if (value <= static_cast<unsigned long long>(std::numeric_limits<int64_t>::max())) {
                    return doof::Success<doof::JsonValue>{doof::JsonValue(static_cast<int64_t>(value))};
                }
            }
        }
        return parse_float_number(token);
    }
};

inline void append_escaped_string(std::string& out, const std::string& value) {
    static constexpr char HEX[] = "0123456789abcdef";
    out.push_back('"');
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    out += "\\u00";
                    out.push_back(HEX[(ch >> 4) & 0x0F]);
                    out.push_back(HEX[ch & 0x0F]);
                } else {
                    out.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    out.push_back('"');
}

inline std::string format_float(double value) {
    if (!std::isfinite(value)) {
        return "null";
    }
    std::ostringstream out;
    out.precision(std::numeric_limits<double>::max_digits10);
    out << value;
    return out.str();
}

inline void append_stringified(std::string& out, const doof::JsonValue& value) {
    std::visit([&out](const auto& inner) {
        using T = std::decay_t<decltype(inner)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            out += "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            out += inner ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int32_t>
            || std::is_same_v<T, int64_t>) {
            out += std::to_string(inner);
        } else if constexpr (std::is_same_v<T, float>
            || std::is_same_v<T, double>) {
            out += format_float(static_cast<double>(inner));
        } else if constexpr (std::is_same_v<T, std::string>) {
            append_escaped_string(out, inner);
        } else if constexpr (std::is_same_v<T, doof::JsonArray>) {
            out.push_back('[');
            if (inner != nullptr) {
                for (size_t index = 0; index < inner->size(); ++index) {
                    if (index > 0) out.push_back(',');
                    append_stringified(out, (*inner)[index]);
                }
            }
            out.push_back(']');
        } else {
            out.push_back('{');
            if (inner != nullptr) {
                bool first = true;
                for (const auto& [key, item] : *inner) {
                    if (!first) out.push_back(',');
                    first = false;
                    append_escaped_string(out, key);
                    out.push_back(':');
                    append_stringified(out, item);
                }
            }
            out.push_back('}');
        }
    }, doof::json_storage(value));
}

} // namespace doof_json_detail

namespace doof_json {

inline doof::Result<doof::JsonValue, std::string> parse(const std::string& text) {
    return doof_json_detail::Parser{text}.parse_document();
}

inline std::string format(const doof::JsonValue& value) {
    std::string out;
    doof_json_detail::append_stringified(out, value);
    return out;
}

} // namespace doof_json
