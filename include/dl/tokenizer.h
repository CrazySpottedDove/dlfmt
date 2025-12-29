#pragma once
#include "dl/token.h"
#include <cstdarg>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <sstream>
namespace dl {
constexpr char DL_TOKENIZER_EOF                     = '\0';
constexpr int  INVALID_LONG_STRING_DELIMITER_LENGTH = -1;
enum class TokenizeMode
{
    compress,
    format,
};

template<TokenizeMode mode> class Tokenizer
{
public:
    Tokenizer(std::string&& text, const std::string& file_name)
        : file_name_(file_name)
        , text_(std::move(text))
        , position_(0)
        , tokens_()
        , length_(text_.length())
    {
        tokens_.reserve(length_ / 4);
        if (length_ >= 3 && static_cast<unsigned char>(text_[0]) == 0xEF &&
            static_cast<unsigned char>(text_[1]) == 0xBB &&
            static_cast<unsigned char>(text_[2]) == 0xBF) {
            position_ = 3;   // 从第4字节开始 tokenize
        }
        tokenize();
    }
#ifndef NDEBUG
    /**
     * @brief 打印所有的 token
     *
     */
    void Print() const noexcept
    {
        for (const auto& token : tokens_) {
            printf("Type: %-12s, Text: '%s'\n",
                   std::string(magic_enum::enum_name(token.type_)).c_str(),
                   std::string(token.source_).c_str());
        }
    }
#endif
    std::vector<Token>&        getTokens() noexcept { return tokens_; }
    std::vector<CommentToken>& getCommentTokens() noexcept { return comment_tokens_; }

private:
    // 查看当前位置往前看第offset个字符
    char peek(size_t offset = 0) const noexcept
    {
        offset += position_;
        return offset < length_ ? text_[offset] : DL_TOKENIZER_EOF;
    }

    /**
     * @brief Look at the current character without any safety checks
     *
     * @return char
     */
    char peek_trust_me() const noexcept { return text_[position_]; }

    /**
     * @brief Advance the current position by one character
     *
     */
    void step() noexcept { ++position_; }

    /**
     * @brief Advance the current position by step_length characters
     *
     * @param step_length
     */
    void step(size_t step_length) noexcept { position_ += step_length; }

    // 获取当前字符并往前走一位
    char get() noexcept { return position_ < length_ ? text_[position_++] : DL_TOKENIZER_EOF; }

    /**
     * @brief Advance the current position until a newline character is encountered
     *
     */
    void step_till_newline() noexcept{
        while (!finished()) {
            if (get_trust_me() == '\n') {
                ++line_;
                break;
            }
        }
    }

    /**
     * @brief Get the current character without any safety checks and advance the position
     *
     * @return char
     */
    char get_trust_me() noexcept{
        return text_[position_++];
    }

    void tokenize(){
        size_t token_start = 0;
        while (true) {
            // Skip White Space
            while (true) {
                // return when finished
                if (finished()) {
                    return;
                }

                // not finished yet, thus we can use peek_trust_me
                const char c = peek_trust_me();
                if (c == ' ' || c == '\t' || c == '\r') {
                    step();
                }
                else if (c == '\n') {
                    step();
                    ++line_;
                }
                else {
                    break;
                }
            }

            // Not finished yet
            //  update token_start
            token_start = position_;
            if constexpr (mode == TokenizeMode::compress) {
                const char c = peek_trust_me();

                // Parse comments and skip them
                if (c == '-' && peek(1) == '-') {
                    step(2);
                    // Long Comment maybe
                    if (peek() == '[') {
                        step();
                        const int delimiter_length = getLongStringDelimiterLength();
                        if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                            // Normal Comment
                            step_till_newline();
                        }
                        else {
                            // Long Comment
                            getLongString(delimiter_length);
                        }
                    }
                    else {
                        // Normal Comment
                        step_till_newline();
                    }
                }

                // One commit processed, goto next turn
                if (position_ > token_start) {
                    continue;
                }
            }else if constexpr (mode == TokenizeMode::format){
                const char c = peek_trust_me();
                const size_t comment_start_line = line_;
                if (c == '-' && peek(1) == '-') {
                    step(2);
                    if (peek() == '[') {
                        size_t comment_start = position_ - 2;   // 包含 "--"
                        step();
                        const int delimiter_length = getLongStringDelimiterLength();
                        if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                            // Normal comment
                            while (!finished()) {
                                if (peek_trust_me() == '\n') {
                                    ++line_;
                                    break;
                                }
                                step();
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
                            size_t      suffix_len = 1 + delimiter_length + 1;   // ]=]（或更长）
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
                        // Normal comment
                        while (!finished()) {
                            if (peek_trust_me() == '\n') {
                                ++line_;
                                break;
                            }
                            step();
                        }
                    }
                }
                if (position_ > token_start) {
                    comment_tokens_.emplace_back(CommentToken{
                        text_.substr(token_start, position_ - token_start), comment_start_line});
                    token_start = position_;
                    continue;
                }
            }

            if (finished()) {
                return;
            }

            // not finished yet
            const char c1 = get_trust_me();

            // String Literal (\n not allowed)
            if (c1 == '\'' || c1 == '\"') {
                while (true) {
                    if (finished()) {
                        error("String literal not closed");
                    }

                    const char c2 = get_trust_me();
                    if (c2 == '\n') {
                        error("String killed by '\\n'");
                    }
                    // escape protect the string literal from getting killed by \n
                    if (c2 == '\\') {
                        if (finished()) {
                            error("String literal not closed");
                        }
                        if (get_trust_me() == '\n') {
                            ++line_;
                        }
                        continue;
                    }
                    if (c2 == c1) {
                        // string closed
                        break;
                    }
                }
                addToken(TokenType::String, token_start);
                continue;
            }

            // Identifier or Keyword
            if (is_identifier_start_char(c1)) {
                while (is_identifier_char(peek())) {
                    step();
                }
                if (is_keyword(text_.substr(token_start, position_ - token_start))) {
                    addToken(TokenType::Keyword, token_start);
                }
                else {
                    addToken(TokenType::Identifier, token_start);
                }
                continue;
            }

            // Variadic symbol "..."
            if (c1 == '.' && peek() == '.' && peek(1) == '.') {
                step(2);
                // Variadic symbol "..." , treat as special identifier
                addToken(TokenType::Identifier, token_start);
                continue;
            }

            // Number
            if (is_digit_char(c1)) {
                // hex
                if (c1 == '0' && (peek() == 'x')) {
                    step();
                    while (is_hex_digit_char(peek())) {
                        step();
                    }
                    addToken(TokenType::Number, token_start);
                    continue;
                }
                // decimals
                else {
                    while (is_digit_char(peek())) {
                        step();
                    }
                    if (finished()) {
                        addToken(TokenType::Number, token_start);
                        continue;
                    }
                    if (peek_trust_me() == '.') {
                        step();
                        while (is_digit_char(peek())) {
                            step();
                        }
                    }

                    if (finished()) {
                        addToken(TokenType::Number, token_start);
                        continue;
                    }

                    const char e_char = peek_trust_me();
                    if (e_char == 'e' || e_char == 'E') {
                        step();
                        if (finished()) {
                            error("exponent part incomplete in number literal");
                        }
                        const char sign_char = peek_trust_me();
                        if (sign_char == '-' || sign_char == '+') {
                            step();
                        }
                        if (finished()) {
                            error("exponent part incomplete in number literal");
                        }
                        const char digit_char = peek_trust_me();
                        if (!is_digit_char(digit_char)) {
                            error("exponent part incomplete in number literal");
                        }
                        step();
                        while (is_digit_char(peek())) {
                            step();
                        }
                    }

                    addToken(TokenType::Number, token_start);
                    continue;
                }
            }

            // Number starting with '.'
            if (c1 == '.' && is_digit_char(peek())) {
                step();
                while (is_digit_char(peek())) {
                    step();
                }
                if (finished()) {
                    addToken(TokenType::Number, token_start);
                    continue;
                }

                const char e_char = peek_trust_me();
                if (e_char == 'e' || e_char == 'E') {
                    step();
                    if (finished()) {
                        error("exponent part incomplete in number literal");
                    }
                    const char sign_char = peek_trust_me();
                    if (sign_char == '-' || sign_char == '+') {
                        step();
                    }
                    if (finished()) {
                        error("exponent part incomplete in number literal");
                    }
                    const char digit_char = peek_trust_me();
                    if (!is_digit_char(digit_char)) {
                        error("exponent part incomplete in number literal");
                    }
                    step();
                    while (is_digit_char(peek())) {
                        step();
                    }
                }

                addToken(TokenType::Number, token_start);
                continue;
            }

            // Long String Maybe
            if (c1 == '[') {
                const int delimiter_length = getLongStringDelimiterLength();
                if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
                    // Single character '['
                    addToken(TokenType::Symbol, token_start);
                }
                else {
                    // Long String
                    getLongString(delimiter_length);
                    addToken(TokenType::String, token_start);
                }
                continue;
            }

            // .. or .
            if (c1 == '.') {
                if (peek() == '.') {
                    get_trust_me();
                }
                addToken(TokenType::Symbol, token_start);
                continue;
            }

            // ==, ~=, <=, >= or single char symbol
            if (is_equal_symbol_char(c1)) {
                if (peek() == '=') {
                    ++position_;
                }
                addToken(TokenType::Symbol, token_start);
                continue;
            }

            // label start/end "::"
            if (c1 == ':' && peek() == ':') {
                ++position_;
                addToken(TokenType::Symbol, token_start);
                continue;
            }

            // Other single char symbols
            if (is_symbol_char(c1)) {
                addToken(TokenType::Symbol, token_start);
                continue;
            }
            error("Bad Symbol %c in source code", c1);
        }
    }
    // void tokenize_compile();
    // void tokenize_compress_mode_core();
    // void tokenize_format_mode_core();
    // void tokenize_format();

    void addToken(const TokenType type, const size_t start_idx) noexcept{
        tokens_.emplace_back(
            Token{type, std::string_view{text_.data() + start_idx, position_ - start_idx}, line_});
    }

    bool finished() const noexcept{
        return position_ >= length_;
    }
    /**
     * @brief Get the long string delimiter length object
     *
     * @return int
     * @note 在已经消费了一个 '[' 时调用
     * @note 如果是长字符串的开始，消费完长字符串的起始定界符并返回定界符长度
     * @note 如果不是长字符串的开始，返回 INVALID_LONG_STRING_DELIMITER_LENGTH，且不消费任何字符
     */
    int getLongStringDelimiterLength() noexcept{
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

    /**
     * @brief Get the long string object
     *
     * @param delimiter_length
     * @note 若读到 EOF，抛出错误
     */
    void getLongString(const int delimiter_length){
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
                for (int i = 0; i < delimiter_length; ++i) {
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

    // 接收类似于 printf 接收的参数
    void error(const char* fmt, ...) const{
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

    std::string               file_name_;
    std::string               text_;
    size_t                    position_ = 0;
    std::vector<Token>        tokens_;
    std::vector<CommentToken> comment_tokens_;
    size_t                    length_ = 0;
    size_t                    line_   = 1;
};
}   // namespace dl