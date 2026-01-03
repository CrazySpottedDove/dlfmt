#pragma once
#include "dl/arena.h"
#include "dl/ast.h"
#include "dl/token.h"
#include <vector>

namespace dl {
class AstManager
{
public:
	// 基本表达式
	AstNode* MakeParenExpr(AstNode* expr, Token* token_open_paren)
	{
		return ast_arena_.emplace(AstNode::ParenExpr{expr}, token_open_paren);
	}
	AstNode* MakeVariableExpr(Token* token_variable)
	{
		return ast_arena_.emplace(AstNode::VariableExpr{token_variable});
	}

	// 复合结构
	AstNode* MakeTableLiteral(std::vector<AstNode::TableEntry>&& entries, Token* token_open_brace,
							  Token* token_close_brace)
	{
		return ast_arena_.emplace(AstNode::TableLiteral{std::move(entries), token_close_brace},
								  token_open_brace);
	}
	AstNode* MakeFunctionLiteral(std::vector<Token*>* args, AstNode* body, Token* token_function,
								 Token* token_end)
	{
		return ast_arena_.emplace(AstNode::FunctionLiteral{args, body, token_end}, token_function);
	}
	AstNode* MakeFunctionStat(std::vector<Token*>* name_chain, std::vector<Token*>* args,
							  AstNode* body, Token* token_function, Token* token_end,
							  bool is_method)
	{
		return ast_arena_.emplace(
			AstNode::FunctionStat{name_chain, args, body, token_end, is_method}, token_function);
	}
	AstNode* MakeArgCall(std::vector<AstNode*>* args, Token* token_open_paren)
	{
		return ast_arena_.emplace(AstNode::ArgCall{args}, token_open_paren);
	}
	AstNode* MakeTableCall(AstNode* table_expr)
	{
		return ast_arena_.emplace(AstNode::TableCall{table_expr}, table_expr->first_token_);
	}
	AstNode* MakeStringCall(Token* token_string)
	{
		return ast_arena_.emplace(AstNode::StringCall{}, token_string);
	}
	AstNode* MakeFieldExpr(AstNode* base, Token* field)
	{
		return ast_arena_.emplace(AstNode::FieldExpr{base, field}, base->first_token_);
	}
	AstNode* MakeMethodExpr(AstNode* base, Token* method, AstNode* func_args)
	{
		return ast_arena_.emplace(AstNode::MethodExpr{base, method, func_args}, base->first_token_);
	}
	AstNode* MakeIndexExpr(AstNode* base, AstNode* index)
	{
		return ast_arena_.emplace(AstNode::IndexExpr{base, index}, base->first_token_);
	}
	AstNode* MakeCallExpr(AstNode* base, AstNode* func_args)
	{
		return ast_arena_.emplace(AstNode::CallExpr{base, func_args}, base->first_token_);
	}

	// 字面量
	AstNode* MakeNumberLiteral(Token* token_number)
	{
		return ast_arena_.emplace(AstNode::NumberLiteral{}, token_number);
	}
	AstNode* MakeStringLiteral(Token* token_string)
	{
		return ast_arena_.emplace(AstNode::StringLiteral{}, token_string);
	}
	AstNode* MakeNilLiteral(Token* token_nil)
	{
		return ast_arena_.emplace(AstNode::NilLiteral{}, token_nil);
	}
	AstNode* MakeBooleanLiteral(Token* token_nil)
	{
		return ast_arena_.emplace(AstNode::BooleanLiteral{}, token_nil);
	}
	AstNode* MakeVargLiteral(Token* token_varg)
	{
		return ast_arena_.emplace(AstNode::VargLiteral{}, token_varg);
	}

	// 一元表达式
	AstNode* MakeNotExpr(AstNode* rhs, Token* token_not)
	{
		return ast_arena_.emplace(AstNode::NotExpr{rhs}, token_not);
	}
	AstNode* MakeNegativeExpr(AstNode* rhs, Token* token_negative)
	{
		return ast_arena_.emplace(AstNode::NegativeExpr{rhs}, token_negative);
	}
	AstNode* MakeLengthExpr(AstNode* rhs, Token* token_pound)
	{
		return ast_arena_.emplace(AstNode::LengthExpr{rhs}, token_pound);
	}

	// 二元表达式
	AstNode* MakeAddExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::AddExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeSubExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::SubExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeMulExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::MulExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeDivExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::DivExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakePowExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::PowExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeModExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::ModExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeConcatExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::ConcatExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeEqExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::EqExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeNeqExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::NeqExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeLtExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::LtExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeLeExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::LeExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeGtExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::GtExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeGeExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::GeExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeAndExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::AndExpr{lhs, rhs}, lhs->first_token_);
	}
	AstNode* MakeOrExpr(AstNode* lhs, AstNode* rhs)
	{
		return ast_arena_.emplace(AstNode::OrExpr{lhs, rhs}, lhs->first_token_);
	}

	// 语句
	AstNode* MakeCallExprStat(AstNode* expr)
	{
		return ast_arena_.emplace(AstNode::CallExprStat{expr}, expr->first_token_);
	}
	AstNode* MakeAssignmentStat(std::vector<AstNode*>* lhs, std::vector<AstNode*>* rhs)
	{
		return ast_arena_.emplace(AstNode::AssignmentStat{lhs, rhs}, (*lhs)[0]->first_token_);
	}
	AstNode* MakeIfStat(AstNode* cond, AstNode* body,
						std::vector<AstNode::IfStat::GeneralElseClause>* else_clauses,
						Token* token_if, Token* token_end)
	{
		return ast_arena_.emplace(AstNode::IfStat{cond, body, else_clauses, token_end}, token_if);
	}
	AstNode* MakeDoStat(AstNode* body, Token* token_do, Token* token_end)
	{
		return ast_arena_.emplace(AstNode::DoStat{body, token_end}, token_do);
	}
	AstNode* MakeWhileStat(AstNode* cond, AstNode* body, Token* token_while, Token* token_end)
	{
		return ast_arena_.emplace(AstNode::WhileStat{cond, body, token_end}, token_while);
	}
	AstNode* MakeNumericForStat(std::vector<Token*>* vars, std::vector<AstNode*>* range,
								AstNode* body, Token* token_for, Token* token_end)
	{
		return ast_arena_.emplace(AstNode::NumericForStat{vars, range, body, token_end}, token_for);
	}
	AstNode* MakeGenericForStat(std::vector<Token*>* vars, std::vector<AstNode*>* gens,
								AstNode* body, Token* token_for, Token* token_end)
	{
		return ast_arena_.emplace(AstNode::GenericForStat{vars, gens, body, token_end}, token_for);
	}
	AstNode* MakeRepeatStat(AstNode* body, AstNode* cond, Token* token_repeat, Token* token_until)
	{
		return ast_arena_.emplace(AstNode::RepeatStat{body, token_until, cond}, token_repeat);
	}
	AstNode* MakeLocalFunctionStat(AstNode* func_stat, Token* token_local)
	{
		return ast_arena_.emplace(AstNode::LocalFunctionStat{func_stat}, token_local);
	}
	AstNode* MakeLocalVarStat(std::vector<Token*>* vars, std::vector<AstNode*>* exprs,
							  Token* token_local)
	{
		return ast_arena_.emplace(AstNode::LocalVarStat{vars, exprs}, token_local);
	}
	AstNode* MakeReturnStat(std::vector<AstNode*>* exprs, Token* token_return)
	{
		return ast_arena_.emplace(AstNode::ReturnStat{exprs}, token_return);
	}
	AstNode* MakeBreakStat(Token* token_break)
	{
		return ast_arena_.emplace(AstNode::BreakStat{}, token_break);
	}
	AstNode* MakeStatList(std::vector<AstNode*>* stats)
	{
		return ast_arena_.emplace(AstNode::StatList{stats},
								  stats->empty() ? nullptr : (*stats)[0]->first_token_);
	}
	AstNode* MakeGotoStat(Token* label, Token* token_goto)
	{
		return ast_arena_.emplace(AstNode::GotoStat{label}, token_goto);
	}
	AstNode* MakeLabelStat(Token* label, Token* token_label_start)
	{
		return ast_arena_.emplace(AstNode::LabelStat{label}, token_label_start);
	}
	std::vector<Token*>*   MakeTokenVector() { return token_vector_arena_.emplace(); }
	std::vector<AstNode*>* MakeAstNodeVector() { return ast_node_vector_arena_.emplace(); }
	std::vector<AstNode::IfStat::GeneralElseClause>* MakeGeneralElseClauseVector()
	{
		return general_else_clause_vector_arena_.emplace();
	}

	void Clear()
	{
		ast_arena_.clear();
		token_vector_arena_.clear();
		ast_node_vector_arena_.clear();
		general_else_clause_vector_arena_.clear();
	}

private:
	Arena<AstNode, 2048>                                        ast_arena_;
	Arena<std::vector<Token*>, 1024>                            token_vector_arena_;
	Arena<std::vector<AstNode*>, 1024>                          ast_node_vector_arena_;
	Arena<std::vector<AstNode::IfStat::GeneralElseClause>, 1024> general_else_clause_vector_arena_;
};
}   // namespace dl