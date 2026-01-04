#include "dl/parser.h"
#include "dl/ast.h"
#include "dl/token.h"
#include <cstddef>
#include <cstdio>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace dl;

void Parser::step() noexcept
{
	if (position_ < tokens_.size() - 1) {
		++position_;
	}
	else {
		reached_eof_ = true;
	}
}

void Parser::step_trust_me() noexcept
{
	++position_;
}

Token* Parser::get() noexcept
{
	Token* token = &tokens_[position_];
	if (position_ < tokens_.size() - 1) {
		++position_;
	}
	else {
		reached_eof_ = true;
	}
	return token;
}

Token* Parser::peek(size_t offset) const noexcept
{
	offset += position_;
	return offset < tokens_.size() ? &tokens_[offset] : &tokens_.back();
}

Token* Parser::peek() const noexcept
{
	return &tokens_[position_];
}

std::string Parser::get_token_start_position(const Token* token) const noexcept
{
	return fmt::format("{}:{}:", file_name_, token->line_);
}

bool Parser::is_block_follow() const noexcept
{
	if (reached_eof_) {
		return true;
	}
	const Token* token = &tokens_[position_];
	return token->type_ == TokenType::Keyword && is_block_follow_keyword(token->source_);
}

bool Parser::is_binop() const noexcept
{
	return is_binop_op(peek()->source_);
}

Token* Parser::expect(TokenType type)
{
	const auto& token = peek();
	if (token->type_ == type) {
		return get();
	}
	SPDLOG_ERROR("Expected token of type {}, but got {} at {}",
				 magic_enum::enum_name(type),
				 magic_enum::enum_name(token->type_),
				 get_token_start_position(token));
	throw std::runtime_error("Unexpected token");
}

void Parser::expect_and_drop(TokenType type)
{
	const auto& token = peek();
	if (token->type_ == type) {
		step();
		return;
	}
	SPDLOG_ERROR("Expected token of type {}, but got {} at {}",
				 magic_enum::enum_name(type),
				 magic_enum::enum_name(token->type_),
				 get_token_start_position(token));
	throw std::runtime_error("Unexpected token");
}

Token* Parser::expect(TokenType type, const std::string_view value)
{
	const auto& token = peek();
	if (token->type_ == type && token->source_ == value) {
		return get();
	}
	SPDLOG_ERROR("Expected token of type {} with value '{}', but got {} with value '{}' at {}",
				 magic_enum::enum_name(type),
				 value,
				 magic_enum::enum_name(token->type_),
				 token->source_,
				 get_token_start_position(token));
	throw std::runtime_error("Unexpected token");
}

void Parser::expect_and_drop(TokenType type, const std::string_view value)
{
	const auto& token = peek();
	if (token->type_ == type && token->source_ == value) {
		step();
		return;
	}
	SPDLOG_ERROR("Expected token of type {} with value '{}', but got {} with value '{}' at {}",
				 magic_enum::enum_name(type),
				 value,
				 magic_enum::enum_name(token->type_),
				 token->source_,
				 get_token_start_position(token));
	throw std::runtime_error("Unexpected token");
}

void Parser::error(const std::string_view message)
{
	const auto& token = peek();
	SPDLOG_ERROR(
		"Error at {} {}, token {}", get_token_start_position(token), message, token->source_);
	throw std::runtime_error("Parsing error");
}

void Parser::exprlist(std::vector<AstNode*>& expr_list)
{
	expr_list.push_back(expr());
	while (peek()->source_ == ",") {
		step();
		expr_list.push_back(expr());
	}
}

AstNode* Parser::prefixexpr()
{
	Token* token = peek();
	if (token->source_ == "(") {
		Token*   open_paren = get();
		AstNode* inner      = expr();
		expect_and_drop(TokenType::Symbol, ")");
		return ast_manager_.MakeParenExpr(inner, open_paren);
	}

	if (token->type_ == TokenType::Identifier) {
		return ast_manager_.MakeVariableExpr(get());
	}
	error("Unexpected symbol in prefix expression");
}

AstNode* Parser::tableexpr()
{
	Token*                           open_brace = expect(TokenType::Symbol, "{");
	std::vector<AstNode::TableEntry> entries;

	while (peek()->source_ != "}") {
		if (peek()->source_ == "[") {
			Token* left_bracket = get();
			auto index_expr = expr();
			expect_and_drop(TokenType::Symbol, "]");
			expect_and_drop(TokenType::Symbol, "=");
			auto value_expr = expr();
			entries.emplace_back(AstNode::TableEntry::IndexEntry{left_bracket, index_expr, value_expr});
		}
		else if (peek()->type_ == TokenType::Identifier && peek(1)->source_ == "=") {
			auto field = get();
			step();
			auto value_expr = expr();
			entries.emplace_back(AstNode::TableEntry::FieldEntry{field, value_expr});
		}
		else {
			auto value_expr = expr();
			entries.emplace_back(AstNode::TableEntry::ValueEntry{value_expr});
		}

		if (peek()->source_ == "," || peek()->source_ == ";") {
			step();
		}
		else {
			break;
		}
	}
	Token* token_close_brace = expect(TokenType::Symbol, "}");
	return ast_manager_.MakeTableLiteral(std::move(entries), open_brace, token_close_brace);
}

void Parser::varlist(std::vector<Token*>& var_list)
{
	if (peek()->type_ == TokenType::Identifier) {
		var_list.push_back(get());
	}
	while (peek()->source_ == ",") {
		step();
		auto identifier = expect(TokenType::Identifier);
		var_list.push_back(identifier);
	}
}

void Parser::blockbody(std::string_view terminator, AstNode*& body, Token*& after)
{
	auto _body  = block();
	auto _after = peek();
	if (_after->type_ == TokenType::Keyword && _after->source_ == terminator) {
		step();
		body  = std::move(_body);
		after = _after;
		return;
	}

	error(fmt::format("Expected '{}' to close block", terminator).c_str());
}

AstNode* Parser::funcdecl_anonymous()
{
	auto function_keyword = get();
	expect_and_drop(TokenType::Symbol, "(");
	auto arg_list = ast_manager_.MakeTokenVector();
	varlist(*arg_list);
	expect_and_drop(TokenType::Symbol, ")");
	AstNode* body;
	Token*   end_token;
	blockbody("end", body, end_token);

	return ast_manager_.MakeFunctionLiteral(arg_list, body, function_keyword, end_token);
}

AstNode* Parser::funcdecl_named()
{
	auto function_keyword = get();
	auto  name_chain_ptr = ast_manager_.MakeTokenVector();
	auto& name_chain     = *name_chain_ptr;
	name_chain.push_back(expect(TokenType::Identifier));
	bool is_method = false;
	while (peek()->source_ == ".") {
		step();
		name_chain.push_back(expect(TokenType::Identifier));
	}
	if (peek()->source_ == ":") {
		step();
		name_chain.push_back(expect(TokenType::Identifier));
		is_method = true;
	}
	expect_and_drop(TokenType::Symbol, "(");
	auto arg_list = ast_manager_.MakeTokenVector();

	varlist(*arg_list);
	expect_and_drop(TokenType::Symbol, ")");
	AstNode* body;
	Token*   end_token;
	blockbody("end", body, end_token);
	return ast_manager_.MakeFunctionStat(
		name_chain_ptr, arg_list, body, function_keyword, end_token, is_method);
}

AstNode* Parser::functionargs()
{
	Token* token = peek();
	if (token->source_ == "(") {
		auto  open_paren   = get();
		auto  arg_list_ptr = ast_manager_.MakeAstNodeVector();
		auto& arg_list     = *arg_list_ptr;
		while (peek()->source_ != ")") {
			arg_list.push_back(expr());
			if (peek()->source_ == ",") {
				step();
			}
			else {
				break;
			}
		}
		expect_and_drop(TokenType::Symbol, ")");

		return ast_manager_.MakeArgCall(arg_list_ptr, open_paren);
	}

	if (token->source_ == "{") {
		// return std::make_unique<TableCall>(expr());
		return ast_manager_.MakeTableCall(expr());
	}

	if (token->type_ == TokenType::String) {
		// return std::make_unique<StringCall>(get());
		return ast_manager_.MakeStringCall(get());
	}
	error("Function arguments expected");
}

AstNode* Parser::primaryexpr()
{
	AstNode* base = prefixexpr();
	while (true) {
		Token* token = peek();
		if (token->source_ == ".") {
			step();
			auto field = expect(TokenType::Identifier);
			base       = ast_manager_.MakeFieldExpr(base, field);
		}
		else if (token->source_ == ":") {
			step();
			auto method    = expect(TokenType::Identifier);
			auto func_args = functionargs();
			base           = ast_manager_.MakeMethodExpr(base, method, func_args);
		}
		else if (token->source_ == "{" || token->source_ == "(" ||
				 token->type_ == TokenType::String) {
			base = ast_manager_.MakeCallExpr(base, functionargs());
		}
		else if (token->source_ == "[") {
			step();
			auto index_expr = expr();
			expect_and_drop(TokenType::Symbol, "]");
			base = ast_manager_.MakeIndexExpr(base, index_expr);
		}
		else {
			break;
		}
	}
	return base;
}

AstNode* Parser::simpleexpr()
{
	Token* token = peek();

	if (token->type_ == TokenType::Number) {
		return ast_manager_.MakeNumberLiteral(get());
	}

	if (token->type_ == TokenType::String) {
		return ast_manager_.MakeStringLiteral(get());
	}

	if (token->source_ == "nil") {
		return ast_manager_.MakeNilLiteral(get());
	}

	if (token->source_ == "true" || token->source_ == "false") {
		return ast_manager_.MakeBooleanLiteral(get());
	}

	if (token->source_ == "...") {
		return ast_manager_.MakeVargLiteral(get());
	}

	if (token->source_ == "{") {
		return tableexpr();
	}

	if (token->source_ == "function") {
		return funcdecl_anonymous();
	}

	return primaryexpr();
}

static constexpr size_t binop_priority_1(std::string_view op)
{
	if (op == "+" || op == "-") return 6;
	if (op == "*" || op == "/" || op == "%") return 7;
	if (op == "^") return 10;
	if (op == "..") return 5;
	if (op == "==" || op == "~=" || op == ">" || op == "<" || op == ">=" || op == "<=") return 3;
	if (op == "and") return 2;
	if (op == "or") return 1;
	return 0;
}

AstNode* Parser::subexpr(const size_t priority_limit)
{
	const static std::unordered_map<std::string_view, size_t> binop_priority_2 = {{"+", 6},
																				  {"-", 6},
																				  {"*", 7},
																				  {"/", 7},
																				  {"%", 7},
																				  {"^", 9},
																				  {"..", 4},
																				  {"==", 3},
																				  {"~=", 3},
																				  {">", 3},
																				  {"<", 3},
																				  {">=", 3},
																				  {"<=", 3},
																				  {"and", 2},
																				  {"or", 1}};
	AstNode*                                                  current_node;
	const auto&                                               op = peek()->source_;
	if (op == "not") {
		auto operator_token = get();
		auto ex             = subexpr(UNARY_PRIORITY);
		current_node        = ast_manager_.MakeNotExpr(ex, operator_token);
	}
	else if (op == "-") {
		auto operator_token = get();
		auto ex             = subexpr(UNARY_PRIORITY);
		current_node        = ast_manager_.MakeNegativeExpr(ex, operator_token);
	}
	else if (op == "#") {
		auto operator_token = get();
		auto ex             = subexpr(UNARY_PRIORITY);
		current_node        = ast_manager_.MakeLengthExpr(ex, operator_token);
	}
	else {
		current_node = simpleexpr();
	}

	while (true) {
		const auto& next_op = peek()->source_;
		if (next_op == "+" && binop_priority_1("+") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeAddExpr(current_node, rhs);
		}
		else if (next_op == "-" && binop_priority_1("-") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeSubExpr(current_node, rhs);
		}
		else if (next_op == "*" && binop_priority_1("*") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeMulExpr(current_node, rhs);
		}
		else if (next_op == "/" && binop_priority_1("/") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeDivExpr(current_node, rhs);
		}
		else if (next_op == "%" && binop_priority_1("%") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeModExpr(current_node, rhs);
		}
		else if (next_op == "^" && binop_priority_1("^") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakePowExpr(current_node, rhs);
		}
		else if (next_op == ".." && binop_priority_1("..") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeConcatExpr(current_node, rhs);
		}
		else if (next_op == "==" && binop_priority_1("==") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeEqExpr(current_node, rhs);
		}
		else if (next_op == "~=" && binop_priority_1("~=") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeNeqExpr(current_node, rhs);
		}
		else if (next_op == ">" && binop_priority_1(">") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeGtExpr(current_node, rhs);
		}
		else if (next_op == "<" && binop_priority_1("<") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeLtExpr(current_node, rhs);
		}
		else if (next_op == ">=" && binop_priority_1(">=") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeGeExpr(current_node, rhs);
		}
		else if (next_op == "<=" && binop_priority_1("<=") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeLeExpr(current_node, rhs);
		}
		else if (next_op == "and" && binop_priority_1("and") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeAndExpr(current_node, rhs);
		}
		else if (next_op == "or" && binop_priority_1("or") > priority_limit) {
			auto operator_token = get();
			auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
			current_node        = ast_manager_.MakeOrExpr(current_node, rhs);
		}
		else {
			break;
		}
	}

	return current_node;
}

inline AstNode* Parser::expr()
{
	return subexpr(0);
}

AstNode* Parser::exprstat()
{
	auto ex = primaryexpr();

	if (ex->type_ == AstNodeType::MethodExpr || ex->type_ == AstNodeType::CallExpr) {
		return ast_manager_.MakeCallExprStat(ex);
	}
	auto  lhs_ptr = ast_manager_.MakeAstNodeVector();
	auto& lhs     = *lhs_ptr;
	lhs.push_back(ex);
	while (peek()->source_ == ",") {
		// lhs_separator.push_back(get());
		step();
		auto lhs_expr = primaryexpr();
		if (lhs_expr->type_ == AstNodeType::MethodExpr ||
			lhs_expr->type_ == AstNodeType::CallExpr) {
			error("Bad left-hand side in assignment");
		}
		lhs.push_back(lhs_expr);
	}
	expect_and_drop(TokenType::Symbol, "=");
	auto  rhs_ptr = ast_manager_.MakeAstNodeVector();
	auto& rhs     = *rhs_ptr;
	rhs.push_back(expr());
	while (peek()->source_ == ",") {
		step();
		rhs.push_back(expr());
	}
	return ast_manager_.MakeAssignmentStat(lhs_ptr, rhs_ptr);
}

AstNode* Parser::ifstat()
{
	auto if_token  = get();
	auto condition = expr();
	expect_and_drop(TokenType::Keyword, "then");
	auto  if_body          = block();
	auto  else_clauses_ptr = ast_manager_.MakeGeneralElseClauseVector();
	auto& else_clauses     = *else_clauses_ptr;
	while (peek()->source_ == "elseif" || peek()->source_ == "else") {
		auto else_if_token = get();
		if (else_if_token->source_ == "elseif") {
			auto else_if_condition = expr();
			expect_and_drop(TokenType::Keyword, "then");
			auto else_if_body = block();
			else_clauses.emplace_back(
				AstNode::IfStat::ElseIfClause{else_if_condition}, else_if_body, else_if_token);
		}
		else {
			auto else_body = block();
			else_clauses.emplace_back(AstNode::IfStat::ElseClause{}, else_body, else_if_token);
			break;
		}
	}

	auto end_token = expect(TokenType::Keyword, "end");
	return ast_manager_.MakeIfStat(condition, if_body, else_clauses_ptr, if_token, end_token);
}

AstNode* Parser::dostat()
{
	auto     do_token = get();
	AstNode* body;
	Token*   end_token;
	blockbody("end", body, end_token);
	return ast_manager_.MakeDoStat(body, do_token, end_token);
}

AstNode* Parser::whilestat()
{
	auto while_token = get();
	auto condition   = expr();
	expect_and_drop(TokenType::Keyword, "do");
	AstNode* body;
	Token*   end_token;
	blockbody("end", body, end_token);
	return ast_manager_.MakeWhileStat(condition, body, while_token, end_token);
}

AstNode* Parser::forstat()
{
	auto  for_token     = get();
	auto  loop_vars_ptr = ast_manager_.MakeTokenVector();
	auto& loop_vars     = *loop_vars_ptr;
	varlist(loop_vars);
	if (peek()->source_ == "=") {
		step();
		auto  loop_expr_list_ptr = ast_manager_.MakeAstNodeVector();
		auto& loop_expr_list     = *loop_expr_list_ptr;

		exprlist(loop_expr_list);
		if (loop_expr_list.size() > 3 || loop_expr_list.size() < 2) {
			error("Numeric for loop must have 2 or 3 values for range bounds");
		}
		expect_and_drop(TokenType::Keyword, "do");
		AstNode* body;
		Token*   end_token;
		blockbody("end", body, end_token);
		return ast_manager_.MakeNumericForStat(
			loop_vars_ptr, loop_expr_list_ptr, body, for_token, end_token);
	}

	if (peek()->source_ == "in") {
		step();
		auto  loop_expr_list_ptr = ast_manager_.MakeAstNodeVector();
		auto& loop_expr_list     = *loop_expr_list_ptr;
		exprlist(loop_expr_list);
		expect_and_drop(TokenType::Keyword, "do");
		AstNode* body;
		Token*   end_token;
		blockbody("end", body, end_token);
		return ast_manager_.MakeGenericForStat(
			loop_vars_ptr, loop_expr_list_ptr, body, for_token, end_token);
	}

	error("Expected '=' or 'in' in for statement");
}

AstNode* Parser::repeatstat()
{
	auto     repeat_token = get();
	AstNode* body;
	Token*   until_token;
	blockbody("until", body, until_token);
	auto condition = expr();
	return ast_manager_.MakeRepeatStat(body, condition, repeat_token, until_token);
}

AstNode* Parser::localdecl()
{
	auto local_token = get();

	if (peek()->source_ == "function") {
		auto function_stat = funcdecl_named();
		if (function_stat->function_stat_.name_chain_->size() > 1) {
			error("Invalid function name in local function declaration");
		}
		return ast_manager_.MakeLocalFunctionStat(function_stat, local_token);
	}

	if (peek()->type_ == TokenType::Identifier) {
		auto  var_list_ptr = ast_manager_.MakeTokenVector();
		auto& var_list     = *var_list_ptr;

		varlist(var_list);

		auto  expr_list_ptr = ast_manager_.MakeAstNodeVector();
		auto& expr_list     = *expr_list_ptr;


		if (peek()->source_ == "=") {
			step();
			exprlist(expr_list);
		}
		return ast_manager_.MakeLocalVarStat(var_list_ptr, expr_list_ptr, local_token);
	}

	error("`function` or identifier expected after `local`");
}

AstNode* Parser::retstat()
{
	auto                  return_token = get();
    auto expr_list_ptr   = ast_manager_.MakeAstNodeVector();
	if (!(is_block_follow() || peek()->source_ == ";")) {
		exprlist(*expr_list_ptr);
	}
	return ast_manager_.MakeReturnStat(expr_list_ptr, return_token);
}

AstNode* Parser::breakstat()
{
	auto break_token = get();
	return ast_manager_.MakeBreakStat(break_token);
}

AstNode* Parser::gotostat()
{
	auto goto_token  = get();
	auto label_token = expect(TokenType::Identifier);
	return ast_manager_.MakeGotoStat(label_token, goto_token);
}

AstNode* Parser::labelstat()
{
	auto label_start_token = get();
	auto label_name_token  = expect(TokenType::Identifier);
	expect_and_drop(TokenType::Symbol, "::");
	return ast_manager_.MakeLabelStat(label_name_token, label_start_token);
}

AstNode* Parser::statement(bool& is_last)
{
	Token* token = peek();
	if (token->source_ == "::") {
		is_last = false;
		return labelstat();
	}
	if (token->source_ == "if") {
		is_last = false;
		return ifstat();
	}
	if (token->source_ == "while") {
		is_last = false;
		return whilestat();
	}
	if (token->source_ == "do") {
		is_last = false;
		return dostat();
	}
	if (token->source_ == "for") {
		is_last = false;
		return forstat();
	}
	if (token->source_ == "repeat") {
		is_last = false;
		return repeatstat();
	}
	if (token->source_ == "function") {
		is_last = false;
		return funcdecl_named();
	}
	if (token->source_ == "local") {
		is_last = false;
		return localdecl();
	}
	if (token->source_ == "return") {
		is_last = true;
		return retstat();
	}
	if (token->source_ == "break") {
		is_last = true;
		return breakstat();
	}
	if (token->source_ == "goto") {
		is_last = false;
		return gotostat();
	}
	is_last = false;
	return exprstat();
}

AstNode* Parser::block()
{
    auto statements_ptr = ast_manager_.MakeAstNodeVector();
    auto& statements = *statements_ptr;
	bool                  is_last = false;
	while (!is_last && !is_block_follow()) {
		statements.push_back(statement(is_last));
		if (peek()->source_ == ";" && peek()->type_ == TokenType::Symbol) {
            step();
		}
	}
	return ast_manager_.MakeStatList(statements_ptr);
}

Parser::Parser(std::vector<Token>& tokens, const std::string& file_name)
	: file_name_(file_name)
	, position_(0)
	, tokens_(tokens)
	, reached_eof_(false)

{
	ast_root_ = block();
}
