#include "dl/tokenizer.h"
#include "dl/macros.h"
#include "dl/token.h"
#include <cstdarg>
#include <cstdio>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
using namespace dl;

char Tokenizer::peek(size_t offset) const noexcept
{
    offset += position_;
    return offset < length_ ? text_[offset] : DL_TOKENIZER_EOF;
}

char Tokenizer::get() noexcept
{
    return position_ < length_ ? text_[position_++] : DL_TOKENIZER_EOF;
}

bool Tokenizer::finished() const noexcept
{
    return position_ >= length_;
}

void Tokenizer::addToken(const TokenType type, const size_t start_idx) noexcept
{
    // tokens_.emplace_back(Token{type, text_.substr(start_idx, position_ - start_idx), line_});
    tokens_.emplace_back(
        Token{type, std::string_view{text_.data() + start_idx, position_ - start_idx}, line_});
}

int Tokenizer::getLongStringDelimiterLength() noexcept
{
    size_t init_pos = position_;
    while (peek() == '=') {
        ++position_;
    }
    if (peek() == '[') {
        ++position_;
        return position_ - init_pos - 1;
    }
    position_ = init_pos;
    return INVALID_LONG_STRING_DELIMITER_LENGTH;
}

void Tokenizer::getLongString(const int delimeter_length)
{
    while (true) {
        const char c = get();
        if (c == DL_TOKENIZER_EOF) {
            error("Long string not closed");
        }
        if (c == '\n') {
            ++line_;
        }
        else if (c == ']') {
            bool ready_to_end = true;
            for (int i = 0; i < delimeter_length; ++i) {
                if (peek(i) != '=') {
                    ready_to_end = false;
                    break;
                }
                ++position_;
            }
            if (ready_to_end && peek() == ']') {
                ++position_;
                return;
            }
            if (peek() == '\n') {
                ++line_;
            }
        }
    }
}

Tokenizer::Tokenizer(std::string&& text, const std::string& file_name, const int work_mode)
    : file_name_(file_name)
    , text_(std::move(text))
    , position_(0)
    , tokens_()
    , length_(text_.length())
{
    tokens_.reserve(length_ * 0.25);   // 经验值，预分配四分之一长度的 token 空间
    if (length_ >= 3 && static_cast<unsigned char>(text_[0]) == 0xEF &&
        static_cast<unsigned char>(text_[1]) == 0xBB &&
        static_cast<unsigned char>(text_[2]) == 0xBF) {
        position_ = 3;   // 从第4字节开始 tokenize
    }
    switch (work_mode) {
    case WORK_MODE_FORMAT: tokenize_format(); break;
    case WORK_MODE_COMPILE: tokenize_compile(); break;
    default: break;
    }
}

void Tokenizer::tokenize_compile()
{
    size_t token_start = 0;

    while (true) {
        // 跳过 WhiteSpace
        while (true) {
            if (finished()) {
                break;
            }
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
                ++position_;
            }
            else if (c == '\n') {
                ++position_;
                ++line_;
            }
            else {
                break;
            }
        }

        token_start = position_;

        // 解析注释
        if (finished()) {
            return;
        }
        const char c = peek();

        if (c == '-' && peek(1) == '-') {
            position_ += 2;
            if (peek() == '[') {
                ++position_;
                const int delimiter_length = getLongStringDelimiterLength();
                if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                    // 普通注释
                    while (true) {
                        const char new_char = get();
                        if (new_char == DL_TOKENIZER_EOF) {
                            break;
                        }
                        else if (new_char == '\n') {
                            ++line_;
                            break;
                        }
                    }
                }
                else {
                    // 长注释
                    getLongString(delimiter_length);
                }
            }
            else {
                // 普通注释
                while (true) {
                    const char new_char = get();
                    if (new_char == DL_TOKENIZER_EOF) {
                        break;
                    }
                    else if (new_char == '\n') {
                        ++line_;
                        break;
                    }
                }
            }
        }

        if (position_ > token_start) {
            continue;
        }

        if (finished()) {
            return;
        }

        const char c1 = get();

        if (c1 == '\'' || c1 == '\"') {
            // 字符串字面量
            while (true) {
                if (finished()) {
                    error("String not closed");
                }
                const char c2 = get();
                if(c2 == '\n'){
                    error("String killed by '\\n'");
                }
                if(c2 == '\\'){
                    if(finished()){
                        error("String not closed");
                    }
                    // 通过转义忽略下一个字符的特殊含义
                    const char c3 = get();
                    if(c3 == '\n'){
                        ++line_;
                    }
                    continue;
                }
                if (c2 == c1) {
                    break;
                }
            }
            addToken(TokenType::String, token_start);
            continue;
        }

        if (is_identifier_start_char(c1)) {
            // 标识符或关键字
            while (is_identifier_char(peek())) {
                ++position_;
            }

            if (is_keyword(text_.substr(token_start, position_ - token_start))) {
                addToken(TokenType::Keyword, token_start);
            }
            else {
                addToken(TokenType::Identifier, token_start);
            }
            continue;
        }

        if (c1 == '.' && peek() == '.' && peek(1) == '.') {
            position_ += 2;
            // 表示变参符号 "..."，作为特殊标识符处理
            addToken(TokenType::Identifier, token_start);
            continue;
        }

        // 数字
        if (is_digit_char(c1) || (c1 == '.' && is_digit_char(peek()))) {
            if (c1 == '0' && (peek() == 'x')) {
                ++position_;
                while (is_hex_digit_char(peek())) {
                    ++position_;
                }
            }
            else {
                while (is_digit_char(peek())) {
                    ++position_;
                }
                if (peek() == '.') {
                    ++position_;
                    while (is_digit_char(peek())) {
                        ++position_;
                    }
                }
                if (peek() == 'e' || peek() == 'E') {
                    ++position_;
                    if (peek() == '-' || peek() == '+') {
                        ++position_;
                    }
                    while (is_digit_char(peek())) {
                        ++position_;
                    }
                }
            }
            addToken(TokenType::Number, token_start);
            continue;
        }

        if (c1 == '[') {
            const int delimiter_length = getLongStringDelimiterLength();
            if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                // 不是长字符串，作为单字符处理
                addToken(TokenType::Symbol, token_start);
            }
            else {
                // 长字符串
                getLongString(delimiter_length);
                addToken(TokenType::String, token_start);
            }
            continue;
        }

        if (c1 == '.') {
            if (peek() == '.') {
                get();
            }
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (is_equal_symbol_char(c1)) {
            if (peek() == '=') {
                ++position_;
            }
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (c1 == ':' && peek() == ':') {
            ++position_;
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (is_symbol_char(c1)) {
            addToken(TokenType::Symbol, token_start);
            continue;
        }
        error("Bad Symbol %c in source code", c1);
    }
}

void Tokenizer::tokenize_format()
{
    size_t token_start = 0;
    while (true) {
        // 跳过 WhiteSpace
        while (true) {
            if (finished()) {
                break;
            }
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r') {
                ++position_;
            }
            else if (c == '\n') {
                ++position_;
                ++line_;
            }
            else {
                break;
            }
        }

        token_start = position_;

        // 解析注释
        if (finished()) {
            return;
        }
        const char   c            = peek();
        const size_t comment_line = line_;
        if (c == '-' && peek(1) == '-') {
            position_ += 2;
            if (peek() == '[') {
                size_t comment_start = position_ - 2;   // 包含 "--"
                ++position_;
                const int delimiter_length = getLongStringDelimiterLength();
                if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                    // 普通注释
                    while (true) {
                        const char new_char = peek();
                        if (new_char == DL_TOKENIZER_EOF) {
                            break;
                        }
                        else if (new_char == '\n') {
                            ++line_;
                            break;
                        }
                        ++position_;
                    }
                }
                else {
                    // 长注释，转化成多条短注释
                    getLongString(delimiter_length);
                    size_t      content_end = position_;
                    std::string comment_text =
                        text_.substr(comment_start, content_end - comment_start);

                    // 去掉前缀 "--[=*[", 后缀 "]=*]"
                    size_t      prefix_len = 2 + 1 + delimiter_length + 1;   // --[
                    size_t      suffix_len = 1 + delimiter_length + 1;       // ]=]（或更长）
                    std::string content    = comment_text.substr(
                        prefix_len, comment_text.size() - prefix_len - suffix_len);

                    // 按行切分
                    std::istringstream iss(content);
                    std::string        line;
                    size_t             divided_comment_line = line_;
                    while (std::getline(iss, line)) {
                        if (line != "") {
                            comment_tokens_.emplace_back(
                                CommentToken{"-- " + line, divided_comment_line});
                            ++divided_comment_line;
                        }
                    }
                    line_       = divided_comment_line;   // 更新全局行号
                    token_start = position_;
                    continue;
                }
            }
            else {
                // 普通注释
                while (true) {
                    const char new_char = peek();
                    if (new_char == DL_TOKENIZER_EOF) {
                        break;
                    }
                    else if (new_char == '\n') {
                        ++line_;
                        break;
                    }
                    ++position_;
                }
            }
        }

        if (position_ > token_start) {
            comment_tokens_.emplace_back(
                CommentToken{text_.substr(token_start, position_ - token_start), comment_line});
            token_start = position_;
            continue;
        }

        if (finished()) {
            return;
        }
        const char c1 = get();

        if (c1 == '\'' || c1 == '\"') {
            // 字符串字面量
            while (true) {
                if (finished()) {
                    error("String not closed");
                }
                const char c2 = get();
                if (c2 == '\n') {
                    error("String killed by '\\n'");
                }
                if (c2 == '\\') {
                    if (finished()) {
                        error("String not closed");
                    }
                    // 通过转义忽略下一个字符的特殊含义
                    const char c3 = get();
                    if (c3 == '\n') {
                        ++line_;
                    }
                    continue;
                }
                if (c2 == c1) {
                    break;
                }
            }
            addToken(TokenType::String, token_start);
            continue;
        }

        if (is_identifier_start_char(c1)) {
            // 标识符或关键字
            while (is_identifier_char(peek())) {
                ++position_;
            }

            if (is_keyword(text_.substr(token_start, position_ - token_start))) {
                addToken(TokenType::Keyword, token_start);
            }
            else {
                addToken(TokenType::Identifier, token_start);
            }
            continue;
        }

        if (c1 == '.' && peek() == '.' && peek(1) == '.') {
            position_ += 2;
            // 表示变参符号 "..."，作为特殊标识符处理
            addToken(TokenType::Identifier, token_start);
            continue;
        }

        // 数字
        if (is_digit_char(c1) || (c1 == '.' && is_digit_char(peek()))) {
            if (c1 == '0' && (peek() == 'x')) {
                ++position_;
                while (is_hex_digit_char(peek())) {
                    ++position_;
                }
            }
            else {
                while (is_digit_char(peek())) {
                    ++position_;
                }
                if (peek() == '.') {
                    ++position_;
                    while (is_digit_char(peek())) {
                        ++position_;
                    }
                }
                if (peek() == 'e' || peek() == 'E') {
                    ++position_;
                    if (peek() == '-' || peek() == '+') {
                        ++position_;
                    }
                    while (is_digit_char(peek())) {
                        ++position_;
                    }
                }
            }
            addToken(TokenType::Number, token_start);
            continue;
        }

        if (c1 == '[') {
            const int delimiter_length = getLongStringDelimiterLength();
            if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                // 不是长字符串，作为单字符处理
                addToken(TokenType::Symbol, token_start);
            }
            else {
                // 长字符串
                getLongString(delimiter_length);
                addToken(TokenType::String, token_start);
            }
            continue;
        }

        if (c1 == '.') {
            if (peek() == '.') {
                get();
            }
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (is_equal_symbol_char(c1)) {
            if (peek() == '=') {
                ++position_;
            }
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (c1 == ':' && peek() == ':') {
            ++position_;
            addToken(TokenType::Symbol, token_start);
            continue;
        }

        if (is_symbol_char(c1)) {
            addToken(TokenType::Symbol, token_start);
            continue;
        }
        error("Bad Symbol %c in source code", c1);
    }
}

void Tokenizer::Print() const noexcept
{
    for (const auto& token : tokens_) {
        printf("Type: %-12s, Text: '%s'\n",
               std::string(magic_enum::enum_name(token.type_)).c_str(),
               std::string(token.source_).c_str());
    }
}

void Tokenizer::error(const char* fmt, ...) const
{
    SPDLOG_ERROR("Tokenizer Error at {}:{}", file_name_, line_);
    char    buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    SPDLOG_ERROR("{}", buf);

    // 尝试打印上一条 token 和 这一条 token
    if (!tokens_.empty()) {
        const Token& last_token = tokens_.back();
        SPDLOG_ERROR("Last Token: Type: {}, Text: {}",
                     std::string(magic_enum::enum_name(last_token.type_)),
                     last_token.source_);
    }

    throw std::runtime_error("Tokenizer Error");
}
