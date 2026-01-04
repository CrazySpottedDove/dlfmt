#pragma once
#include "dl/token.h"
#include <cstdarg>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>
namespace dl {
constexpr char DL_TOKENIZER_EOF                     = '\0';
constexpr int  INVALID_LONG_STRING_DELIMITER_LENGTH = -1;
enum class TokenizeMode
{
	Compress,
	FormatAuto,
	FormatManual
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
		for (const auto& comment_token : comment_tokens_) {
			printf("Comment Type: %-12s, Text: '%s'\n",
				   std::string(magic_enum::enum_name(comment_token.type_)).c_str(),
				   std::string(comment_token.source_).c_str());
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
	 * @brief Advance the current position until a newline character is encountered. The newline
	 * character is not consumed, so the current position will be at the newline character after
	 * this function is called.
	 *
	 */
	void step_till_newline() noexcept
	{
		while (!finished()) {
			if (peek_trust_me() == '\n') {
				break;
			}
			step();
		}
	}

	/**
	 * @brief Get the current character without any safety checks and advance the position
	 *
	 * @return char
	 */
	char get_trust_me() noexcept { return text_[position_++]; }

	void tokenize()
	{
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
					if constexpr (mode == TokenizeMode::FormatManual) {
						bool empty_line_detected = false;
						char next_char           = peek();
						while (true) {
							if (next_char == '\n') {
								empty_line_detected = true;
								++line_;
							}
							else if (!(next_char == ' ' || next_char == '\t' ||
									   next_char == '\r')) {
								break;
							}
							step();
							next_char = peek();
						}
						if (empty_line_detected) {
							// For Empty Line, string_view is useless, so just give it an empty
							// string
							comment_tokens_.emplace_back(std::string_view{text_.data(), 0},
														 line_ - 1,
														 CommentTokenType::EmptyLine);
						}
						if (finished()) {
							return;
						}
					}
				}
				else {
					break;
				}
			}

			// Not finished yet
			//  update token_start
			token_start = position_;
			if constexpr (mode == TokenizeMode::Compress) {
				// Parse comments and skip them
				if (peek_trust_me() == '-' && peek(1) == '-') {
					step(2);
					// Long Comment maybe
					if (peek() == '[') {
						step();
						const int delimiter_length = getLongStringDelimiterLength();
						if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
							// Normal Comment
							step_till_newline();
							step();   // skip \n
							++line_;
						}
						else {
							// Long Comment
							getLongString(delimiter_length);
						}
					}
					else {
						// Normal Comment
						step_till_newline();
						step();   // skip \n
						++line_;
					}
					// skip the comment token
					continue;
				}
			}
			else {
				if (peek_trust_me() == '-' && peek(1) == '-') {
					step(2);
					if (peek() == '[') {
						step();
						const int delimiter_length = getLongStringDelimiterLength();
						// normal comment
						if (delimiter_length == INVALID_LONG_STRING_DELIMITER_LENGTH) {
							// Normal comment
							step_till_newline();
							// \n not included
							addCommentToken(CommentTokenType::ShortComment, token_start);
						}
						// long comment
						else {
							getLongString(delimiter_length);
							// also the last \n is not included
							addCommentToken(CommentTokenType::LongComment, token_start);
						}
					}
					else {
						// Normal comment
						step_till_newline();
						addCommentToken(CommentTokenType::ShortComment, token_start);
					}
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
				if (is_keyword(
						std::string_view(text_.data() + token_start, position_ - token_start))) {
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

	void addToken(const TokenType type, const size_t start_idx) noexcept
	{
		tokens_.emplace_back(
			std::string_view{text_.data() + start_idx, position_ - start_idx}, line_, type);
	}

	void addCommentToken(const CommentTokenType type, const size_t start_idx) noexcept
	{
		comment_tokens_.emplace_back(
			std::string_view{text_.data() + start_idx, position_ - start_idx}, line_, type);
	}

	bool finished() const noexcept { return position_ >= length_; }
	/**
	 * @brief Get the long string delimiter length object
	 *
	 * @return int
	 * @note 在已经消费了一个 '[' 时调用
	 * @note 如果是长字符串的开始，消费完长字符串的起始定界符并返回定界符长度
	 * @note 如果不是长字符串的开始，返回 INVALID_LONG_STRING_DELIMITER_LENGTH，且不消费任何字符
	 */
	int getLongStringDelimiterLength() noexcept
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

	/**
	 * @brief Get the long string object
	 *
	 * @param delimiter_length
	 * @note 若读到 EOF，抛出错误
	 */
	void getLongString(const int delimiter_length)
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
	void error(const char* fmt, ...) const
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

	std::string               file_name_;
	std::string               text_;
	size_t                    position_ = 0;
	std::vector<Token>        tokens_;
	std::vector<CommentToken> comment_tokens_;
	size_t                    length_ = 0;
	size_t                    line_   = 1;
};
}   // namespace dl