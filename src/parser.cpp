#include "dl/parser.h"
#include "dl/token.h"
#include <cstddef>
#include <cstdio>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
using namespace dl;

Token* Parser::get() noexcept
{
    Token* token = &tokens_.at(position_);
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
    return offset < tokens_.size() ? &tokens_.at(offset) : &tokens_.back();
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
    const Token* token = &tokens_.at(position_);
    return token->type_ == TokenType::Keyword && is_block_follow_keyword(token->source_);
}

bool Parser::is_unop() const noexcept
{
    return is_unop_op(peek()->source_);
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

void Parser::error(const std::string_view message)
{
    const auto& token = peek();
    SPDLOG_ERROR(
        "Error at {} {}, token {}", get_token_start_position(token), message, token->source_);
    throw std::runtime_error("Parsing error");
}

void Parser::exprlist(std::vector<std::unique_ptr<AstNode>>& expr_list,
                      std::vector<Token*>&                   comma_list)
{
    expr_list.push_back(expr());
    while (peek()->source_ == ",") {
        comma_list.push_back(get());
        expr_list.push_back(expr());
    }
}

std::unique_ptr<AstNode> Parser::prefixexpr()
{
    Token* token = peek();
    if (token->source_ == "(") {
        Token*                   open_paren  = get();
        std::unique_ptr<AstNode> inner       = expr();
        Token*                   close_paren = expect(TokenType::Symbol, ")");
        return std::make_unique<ParenExpr>(std::move(inner), open_paren, close_paren);
    }

    if (token->type_ == TokenType::Identifier) {
        return std::make_unique<VariableExpr>(get());
    }
    error("Unexpected symbol in prefix expression");
}

std::unique_ptr<AstNode> Parser::tableexpr()
{
    Token*                                   open_brace = expect(TokenType::Symbol, "{");
    std::vector<std::unique_ptr<TableEntry>> entries;
    std::vector<Token*>                      seperators;

    while (peek()->source_ != "}") {
        if (peek()->source_ == "[") {
            auto open_bracket  = get();
            auto index_expr    = expr();
            auto close_bracket = expect(TokenType::Symbol, "]");
            auto equal_token   = expect(TokenType::Symbol, "=");
            auto value_expr    = expr();
            entries.emplace_back(std::make_unique<IndexEntry>(std::move(index_expr),
                                                              std::move(value_expr),
                                                              open_bracket,
                                                              close_bracket,
                                                              equal_token));
        }
        else if (peek()->type_ == TokenType::Identifier && peek(1)->source_ == "=") {
            auto field       = get();
            auto equal_token = get();
            auto value_expr  = expr();
            entries.emplace_back(
                std::make_unique<FieldEntry>(field, std::move(value_expr), equal_token));
        }
        else {
            auto value_expr = expr();
            entries.emplace_back(std::make_unique<ValueEntry>(std::move(value_expr)));
        }

        if (peek()->source_ == "," || peek()->source_ == ";") {
            seperators.push_back(get());
        }
        else {
            break;
        }
    }
    Token* close_brace = expect(TokenType::Symbol, "}");
    return std::make_unique<TableLiteral>(
        std::move(entries), std::move(seperators), open_brace, close_brace);
}

void Parser::varlist(std::vector<Token*>& var_list, std::vector<Token*>& comma_list)
{
    if (peek()->type_ == TokenType::Identifier) {
        var_list.push_back(get());
    }
    while (peek()->source_ == ",") {
        comma_list.push_back(get());
        auto identifier = expect(TokenType::Identifier);
        var_list.push_back(identifier);
    }
}

void Parser::blockbody(std::string_view terminator, std::unique_ptr<AstNode>& body, Token*& after)
{
    auto _body  = block();
    auto _after = peek();
    if (_after->type_ == TokenType::Keyword && _after->source_ == terminator) {
        get();
        body  = std::move(_body);
        after = _after;
        return;
    }

    error(fmt::format("Expected '{}' to close block", terminator).c_str());
}

std::unique_ptr<AstNode> Parser::funcdecl_anonymous()
{
    auto                function_keyword = get();
    auto                open_paren       = expect(TokenType::Symbol, "(");
    std::vector<Token*> arg_list;
    std::vector<Token*> comma_list;
    varlist(arg_list, comma_list);
    auto                     close_paren = expect(TokenType::Symbol, ")");
    std::unique_ptr<AstNode> body;
    Token*                   end_token;
    blockbody("end", body, end_token);
    return std::make_unique<FunctionLiteral>(std::move(arg_list),
                                             std::move(body),
                                             function_keyword,
                                             open_paren,
                                             std::move(comma_list),
                                             close_paren,
                                             end_token);
}

std::unique_ptr<FunctionStat> Parser::funcdecl_named()
{
    auto                function_keyword = get();
    std::vector<Token*> name_chain;
    std::vector<Token*> name_chain_separator;
    name_chain.push_back(expect(TokenType::Identifier));
    while (peek()->source_ == ".") {
        name_chain_separator.push_back(get());
        name_chain.push_back(expect(TokenType::Identifier));
    }
    if (peek()->source_ == ":") {
        name_chain_separator.push_back(get());
        name_chain.push_back(expect(TokenType::Identifier));
    }
    auto                open_paren = expect(TokenType::Symbol, "(");
    std::vector<Token*> arg_list;
    std::vector<Token*> comma_list;
    varlist(arg_list, comma_list);
    auto                     close_paren = expect(TokenType::Symbol, ")");
    std::unique_ptr<AstNode> body;
    Token*                   end_token;
    blockbody("end", body, end_token);
    return std::make_unique<FunctionStat>(std::move(name_chain),
                                          std::move(arg_list),
                                          std::move(body),
                                          function_keyword,
                                          std::move(name_chain_separator),
                                          open_paren,
                                          std::move(comma_list),
                                          close_paren,
                                          end_token);
}

std::unique_ptr<AstNode> Parser::functionargs()
{
    Token* token = peek();
    if (token->source_ == "(") {
        auto                                  open_paren = get();
        std::vector<std::unique_ptr<AstNode>> arg_list;
        std::vector<Token*>                   comma_list;
        while (peek()->source_ != ")") {
            arg_list.push_back(expr());
            if (peek()->source_ == ",") {
                comma_list.push_back(get());
            }
            else {
                break;
            }
        }
        auto close_paren = expect(TokenType::Symbol, ")");
        return std::make_unique<ArgCall>(
            std::move(arg_list), std::move(comma_list), open_paren, close_paren);
    }

    if (token->source_ == "{") {
        return std::make_unique<TableCall>(expr());
    }

    if (token->type_ == TokenType::String) {
        return std::make_unique<StringCall>(get());
    }
    error("Function arguments expected");
}

std::unique_ptr<AstNode> Parser::primaryexpr()
{
    std::unique_ptr<AstNode> base = prefixexpr();
    while (true) {
        Token* token = peek();
        if (token->source_ == ".") {
            auto dot_token = get();
            auto field     = expect(TokenType::Identifier);
            base           = std::make_unique<FieldExpr>(std::move(base), field, dot_token);
        }
        else if (token->source_ == ":") {
            auto colon_token = get();
            auto method      = expect(TokenType::Identifier);
            auto func_args   = functionargs();
            base             = std::make_unique<MethodExpr>(
                std::move(base), method, std::move(func_args), colon_token);
        }
        else if (token->source_ == "{" || token->source_ == "(" ||
                 token->type_ == TokenType::String) {
            base = std::make_unique<CallExpr>(std::move(base), functionargs());
        }
        else if (token->source_ == "[") {
            auto open_bracket  = get();
            auto index_expr    = expr();
            auto close_bracket = expect(TokenType::Symbol, "]");
            base               = std::make_unique<IndexExpr>(
                std::move(base), std::move(index_expr), open_bracket, close_bracket);
        }
        else {
            break;
        }
    }
    return base;
}

std::unique_ptr<AstNode> Parser::simpleexpr()
{
    Token* token = peek();

    if (token->type_ == TokenType::Number) {
        return std::make_unique<NumberLiteral>(get());
    }

    if (token->type_ == TokenType::String) {
        return std::make_unique<StringLiteral>(get());
    }

    if (token->source_ == "nil") {
        return std::make_unique<NilLiteral>(get());
    }

    if (token->source_ == "true" || token->source_ == "false") {
        return std::make_unique<BooleanLiteral>(get());
    }

    if (token->source_ == "...") {
        return std::make_unique<VargLiteral>(get());
    }

    if (token->source_ == "{") {
        return tableexpr();
    }

    if (token->source_ == "function") {
        return funcdecl_anonymous();
    }

    return primaryexpr();
}

std::unique_ptr<AstNode> Parser::subexpr(const size_t priority_limit)
{
    const static std::unordered_map<std::string_view, size_t> binop_priority_1 = {{"+", 6},
                                                                                  {"-", 6},
                                                                                  {"*", 7},
                                                                                  {"/", 7},
                                                                                  {"%", 7},
                                                                                  {"^", 10},
                                                                                  {"..", 5},
                                                                                  {"==", 3},
                                                                                  {"~=", 3},
                                                                                  {">", 3},
                                                                                  {"<", 3},
                                                                                  {">=", 3},
                                                                                  {"<=", 3},
                                                                                  {"and", 2},
                                                                                  {"or", 1}};
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
    std::unique_ptr<AstNode>                                  current_node;
    const auto& op = peek()->source_;
    if(op == "not"){
        auto operator_token = get();
        auto ex             = subexpr(UNARY_PRIORITY);
        current_node        = std::make_unique<NotExpr>(operator_token, std::move(ex));
    }else if(op == "-"){
        auto operator_token = get();
        auto ex             = subexpr(UNARY_PRIORITY);
        current_node        = std::make_unique<NegativeExpr>(operator_token, std::move(ex));
    }else if(op == "#"){
        auto operator_token = get();
        auto ex             = subexpr(UNARY_PRIORITY);
        current_node        = std::make_unique<LengthExpr>(operator_token, std::move(ex));
    }else {
        current_node = simpleexpr();
    }

    while (is_binop() && binop_priority_1.at(peek()->source_) > priority_limit) {
        auto operator_token = get();
        auto rhs            = subexpr(binop_priority_2.at(operator_token->source_));
        current_node =
            std::make_unique<BinopExpr>(std::move(current_node), operator_token, std::move(rhs));
    }
    return current_node;
}

inline std::unique_ptr<AstNode> Parser::expr()
{
    return subexpr(0);
}

std::unique_ptr<AstNode> Parser::exprstat()
{
    auto ex = primaryexpr();

    if (ex->GetType() == AstNodeType::MethodExpr || ex->GetType() == AstNodeType::CallExpr) {
        return std::make_unique<CallExprStat>(std::move(ex));
    }

    std::vector<std::unique_ptr<AstNode>> lhs;
    lhs.push_back(std::move(ex));
    std::vector<Token*> lhs_separator;
    while (peek()->source_ == ",") {
        lhs_separator.push_back(get());
        auto lhs_expr = primaryexpr();
        if (lhs_expr->GetType() == AstNodeType::MethodExpr ||
            lhs_expr->GetType() == AstNodeType::CallExpr) {
            error("Bad left-hand side in assignment");
        }
        lhs.push_back(std::move(lhs_expr));
    }
    auto                                  assign_token = expect(TokenType::Symbol, "=");
    std::vector<std::unique_ptr<AstNode>> rhs;
    rhs.push_back(expr());
    std::vector<Token*> rhs_separator;
    while (peek()->source_ == ",") {
        rhs_separator.push_back(get());
        rhs.push_back(expr());
    }

    return std::make_unique<AssignmentStat>(std::move(lhs),
                                            std::move(rhs),
                                            assign_token,
                                            std::move(lhs_separator),
                                            std::move(rhs_separator));
}

std::unique_ptr<AstNode> Parser::ifstat()
{
    auto                                            if_token   = get();
    auto                                            condition  = expr();
    auto                                            then_token = expect(TokenType::Keyword, "then");
    auto                                            if_body    = block();
    std::vector<std::unique_ptr<GeneralElseClause>> else_clauses;
    while (peek()->source_ == "elseif" || peek()->source_ == "else") {
        auto else_if_token = get();
        if (else_if_token->source_ == "elseif") {
            auto else_if_condition  = expr();
            auto else_if_then_token = expect(TokenType::Keyword, "then");
            auto else_if_body       = block();
            else_clauses.emplace_back(std::make_unique<ElseIfClause>(std::move(else_if_condition),
                                                                     std::move(else_if_body),
                                                                     else_if_token,
                                                                     else_if_then_token));
        }
        else {
            auto else_body = block();
            else_clauses.emplace_back(
                std::make_unique<ElseClause>(std::move(else_body), else_if_token));
            // no more clauses after else
            break;
        }
    }

    auto end_token = expect(TokenType::Keyword, "end");
    return std::make_unique<IfStat>(std::move(condition),
                                    std::move(if_body),
                                    std::move(else_clauses),
                                    if_token,
                                    then_token,
                                    end_token);
}

std::unique_ptr<AstNode> Parser::dostat()
{
    auto                     do_token = get();
    std::unique_ptr<AstNode> body;
    Token*                   end_token;
    blockbody("end", body, end_token);
    return std::make_unique<DoStat>(do_token, end_token, std::move(body));
}

std::unique_ptr<AstNode> Parser::whilestat()
{
    auto                     while_token = get();
    auto                     condition   = expr();
    auto                     do_token    = expect(TokenType::Keyword, "do");
    std::unique_ptr<AstNode> body;
    Token*                   end_token;
    blockbody("end", body, end_token);
    return std::make_unique<WhileStat>(
        std::move(condition), std::move(body), while_token, do_token, end_token);
}

std::unique_ptr<AstNode> Parser::forstat()
{
    auto                for_token = get();
    std::vector<Token*> loop_vars;
    std::vector<Token*> loop_var_commas;
    varlist(loop_vars, loop_var_commas);
    if (peek()->source_ == "=") {
        auto                                  assign_token = get();
        std::vector<std::unique_ptr<AstNode>> loop_expr_list;
        std::vector<Token*>                   loop_expr_commas;
        exprlist(loop_expr_list, loop_expr_commas);
        if (loop_expr_list.size() > 3 || loop_expr_list.size() < 2) {
            error("Numeric for loop must have 2 or 3 values for range bounds");
        }
        auto                     do_token = expect(TokenType::Keyword, "do");
        std::unique_ptr<AstNode> body;
        Token*                   end_token;
        blockbody("end", body, end_token);
        return std::make_unique<NumericForStat>(std::move(loop_vars),
                                                std::move(loop_expr_list),
                                                std::move(body),
                                                for_token,
                                                std::move(loop_var_commas),
                                                assign_token,
                                                std::move(loop_expr_commas),
                                                do_token,
                                                end_token);
    }

    if (peek()->source_ == "in") {
        auto                                  in_token = get();
        std::vector<std::unique_ptr<AstNode>> loop_expr_list;
        std::vector<Token*>                   loop_expr_commas;
        exprlist(loop_expr_list, loop_expr_commas);
        auto                     do_token = expect(TokenType::Keyword, "do");
        std::unique_ptr<AstNode> body;
        Token*                   end_token;
        blockbody("end", body, end_token);
        return std::make_unique<GenericForStat>(std::move(loop_vars),
                                                std::move(loop_expr_list),
                                                std::move(body),
                                                for_token,
                                                std::move(loop_var_commas),
                                                in_token,
                                                std::move(loop_expr_commas),
                                                do_token,
                                                end_token);
    }

    error("Expected '=' or 'in' in for statement");
}

std::unique_ptr<AstNode> Parser::repeatstat()
{
    auto                     repeat_token = get();
    std::unique_ptr<AstNode> body;
    Token*                   until_token;
    blockbody("until", body, until_token);
    auto condition = expr();
    return std::make_unique<RepeatStat>(
        std::move(body), std::move(condition), repeat_token, until_token);
}

std::unique_ptr<AstNode> Parser::localdecl()
{
    auto local_token = get();

    if (peek()->source_ == "function") {
        auto function_stat = funcdecl_named();
        if (function_stat->name_chain_.size() > 1) {
            error("Invalid function name in local function declaration");
        }
        return std::make_unique<LocalFunctionStat>(std::move(function_stat), local_token);
    }

    if (peek()->type_ == TokenType::Identifier) {
        std::vector<Token*> var_list;
        std::vector<Token*> comma_list;
        varlist(var_list, comma_list);

        Token*                                assign_token = nullptr;
        std::vector<std::unique_ptr<AstNode>> expr_list;
        std::vector<Token*>                   expr_comma_list;
        if (peek()->source_ == "=") {
            assign_token = get();
            exprlist(expr_list, expr_comma_list);
        }
        return std::make_unique<LocalVarStat>(std::move(var_list),
                                              std::move(expr_list),
                                              local_token,
                                              assign_token,
                                              std::move(comma_list),
                                              std::move(expr_comma_list));
    }

    error("`function` or identifier expected after `local`");
}

std::unique_ptr<AstNode> Parser::retstat()
{
    auto                                  return_token = get();
    std::vector<std::unique_ptr<AstNode>> expr_list;
    std::vector<Token*>                   comma_list;
    if (!(is_block_follow() || peek()->source_ == ";")) {
        exprlist(expr_list, comma_list);
    }
    return std::make_unique<ReturnStat>(std::move(expr_list), return_token, comma_list);
}

std::unique_ptr<AstNode> Parser::breakstat()
{
    auto break_token = get();
    return std::make_unique<BreakStat>(break_token);
}

std::unique_ptr<AstNode> Parser::gotostat()
{
    auto goto_token  = get();
    auto label_token = expect(TokenType::Identifier);
    return std::make_unique<GotoStat>(goto_token, label_token);
}

std::unique_ptr<AstNode> Parser::labelstat()
{
    auto label_start_token = get();
    auto label_name_token  = expect(TokenType::Identifier);
    auto label_end_token   = expect(TokenType::Symbol, "::");
    return std::make_unique<LabelStat>(label_start_token, label_name_token, label_end_token);
}

std::unique_ptr<AstNode> Parser::statement(bool& is_last)
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

std::unique_ptr<AstNode> Parser::block()
{
    std::vector<std::unique_ptr<AstNode>> statements;
    std::vector<Token*>                   semicolons;
    bool                                  is_last = false;
    while (!is_last && !is_block_follow()) {
        statements.push_back(statement(is_last));
        if (peek()->source_ == ";" && peek()->type_ == TokenType::Symbol) {
            semicolons.push_back(get());
        }
    }
    return std::make_unique<StatList>(std::move(statements), std::move(semicolons));
}

Parser::Parser(std::vector<Token>& tokens, const std::string& file_name)
    : file_name_(file_name)
    , position_(0)
    , tokens_(tokens)
    , reached_eof_(false)

{
    ast_root_ = block();
}
