#pragma once
#include <bitset>
#include <string>
#include <string_view>
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

struct CommentToken
{
	std::string source_;
	std::size_t line_;
};

struct Token
{
	// Token 内容
	std::string_view source_;
	// Token 所在行号
	std::size_t line_;
	// Token 类型
	TokenType type_;
	Token(std::string_view source, std::size_t line, TokenType type)
		: source_(source)
		, line_(line)
		, type_(type)
	{}
};

inline bool is_white_char(const char c)
{
	return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

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

// inline bool is_symbol_char(const char c)
// {
//     const std::unordered_set<char> symbols = {
//         '+', '-', '*', '/', '^', '%', ',', '{', '}', '[', ']', '(', ')', ';', '#', '.', ':'};
//     return symbols.find(c) != symbols.end();
// }

inline bool is_symbol_char(const char c)
{
	static const std::bitset<256> table = [] {
		std::bitset<256> b;
		const char       syms[] = {
            '+', '-', '*', '/', '^', '%', ',', '{', '}', '[', ']', '(', ')', ';', '#', '.', ':'};
		for (char ch : syms) {
			b.set(static_cast<unsigned char>(ch));
		}
		return b;
	}();
	return table.test(static_cast<unsigned char>(c));
}

inline bool is_equal_symbol_char(const char c)
{
	return c == '=' || c == '~' || c == '<' || c == '>';
}

inline bool is_keyword(const std::string_view str)
{
	// if 链写法
	return str == "and" || str == "break" || str == "do" || str == "else" || str == "elseif" ||
		   str == "end" || str == "false" || str == "for" || str == "function" || str == "goto" ||
		   str == "if" || str == "in" || str == "local" || str == "nil" || str == "not" ||
		   str == "or" || str == "repeat" || str == "return" || str == "then" || str == "true" ||
		   str == "until" || str == "while";
}
// ab
inline bool is_block_follow_keyword(const std::string_view str)
{
	return str == "else" || str == "elseif" || str == "end" || str == "until";
}

inline bool is_binop_op(const std::string_view str)
{
	return str == "+" || str == "-" || str == "*" || str == "/" || str == "^" || str == "%" ||
		   str == ".." || str == "==" || str == "~=" || str == "<=" || str == ">=" || str == "<" ||
		   str == ">" || str == "and" || str == "or";
}
}   // namespace dl