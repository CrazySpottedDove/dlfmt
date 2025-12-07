#pragma once
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <string>
namespace dl {
enum class TokenType
{
    Eof,
    Identifier,
    Keyword,
    Number,
    String,
    Symbol,
    Comment,
    WhiteSpace
};

struct CommentToken{
    std::string source_;
    std::size_t line_;
};

struct Token
{
    // Token 类型
    TokenType type_;
    // Token 内容
    std::string_view source_;
    // Token 所在行号
    std::size_t line_;
};

inline bool is_white_char(const char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

const std::unordered_map<char, char> character_for_escape = {
    // {'r', '\r'}, {'n', '\n'}, {'t', '\t'}, {'"', '\"'}, {'\'', '\''}, {'\\', '\\'}, {'f', '\f'}, {'b', '\b'}, {'v', '\v'}
    {'\'','\'' }
};

inline bool is_identifier_start_char(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool is_identifier_char(const char c)
{
    return is_identifier_start_char(c) || (c >= '0' && c <= '9');
}

inline bool is_digit_char(const char c)
{
    return c >= '0' && c <= '9';
}

inline bool is_hex_digit_char(const char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool is_symbol_char(const char c)
{
    const std::unordered_set<char> symbols = {
        '+', '-', '*', '/', '^', '%', ',', '{', '}', '[', ']', '(', ')', ';', '#', '.', ':'};
    return symbols.find(c) != symbols.end();
}

inline bool is_equal_symbol_char(const char c){
    return c == '=' || c == '~' || c == '<' || c == '>';
}

inline bool is_keyword(const std::string_view str){
    static const std::unordered_set<std::string_view> key_words = {
        "and","break","do","else","elseif","end","false","for","function","goto","if","in","local","nil","not","or","repeat","return","then","true","until","while"
    };
    return key_words.find(str) != key_words.end();
}

inline bool is_block_follow_keyword(const std::string_view str){
    static const std::unordered_set<std::string_view> block_follow_keywords = {
        "else","elseif","end","until"
    };
    return block_follow_keywords.find(str) != block_follow_keywords.end();
}

inline bool is_unop_op(const std::string_view str){
    static const std::unordered_set<std::string_view> unops = {
        "not","-","#"
    };
    return unops.find(str) != unops.end();
}

inline bool is_binop_op(const std::string_view str){
    static const std::unordered_set<std::string_view> binops = {
        "+","-","*","/","^","%","..","==","~=","<=",">=","<",">","and","or"
    };
    return binops.find(str) != binops.end();
}
}   // namespace dl