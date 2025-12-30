#pragma once
#include "dl/token.h"
#include <memory>
#include <vector>
namespace dl {
enum class AstNodeType
{
	ParenExpr,
	VariableExpr,
	TableLiteral,
	FunctionLiteral,
	FunctionStat,
	ArgCall,
	TableCall,
	StringCall,
	FieldExpr,
	MethodExpr,
	IndexExpr,
	CallExpr,
	NumberLiteral,
	StringLiteral,
	NilLiteral,
	BooleanLiteral,
	VargLiteral,
	// UnopExpr,
	BinopExpr,
	CallExprStat,
	AssignmentStat,
	IfStat,
	DoStat,
	WhileStat,
	NumericForStat,
	GenericForStat,
	RepeatStat,
	LocalFunctionStat,
	LocalVarStat,
	ReturnStat,
	BreakStat,
	StatList,
	GotoStat,
	LabelStat,
	// UnopExpr types
	NotExpr,
	NegativeExpr,
	LengthExpr,
};

class AstNode
{
public:
	virtual ~AstNode()                                 = default;
	virtual Token*      GetFirstToken() const noexcept = 0;
	virtual Token*      GetLastToken() const noexcept  = 0;
	virtual AstNodeType GetType() const noexcept       = 0;
};

/**
 * @brief 括号表达式
 *
 */
class ParenExpr : public AstNode
{
public:
	ParenExpr(std::unique_ptr<AstNode> expression, Token* token_open_paren,
			  Token* token_close_paren)
		: expression_(std::move(expression))
		, token_open_paren_(token_open_paren)
		, token_close_paren_(token_close_paren)
	{}
	~ParenExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return token_open_paren_; }
	Token*      GetLastToken() const noexcept override { return token_close_paren_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::ParenExpr; }

	std::unique_ptr<AstNode> expression_;
	Token*                   token_open_paren_;
	Token*                   token_close_paren_;
};

/**
 * @brief 变量表达式
 *
 */
class VariableExpr : public AstNode
{
public:
	VariableExpr(Token* token)
		: token_(token)
	{}
	~VariableExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::VariableExpr; }

	Token* token_;
};

enum class TableEntryType
{
	Index,
	Field,
	Value
};

class TableEntry
{
public:
	virtual ~TableEntry()                           = default;
	virtual TableEntryType GetType() const noexcept = 0;
};

/**
 * @brief 表索引项
 *
 */
class IndexEntry : public TableEntry
{
public:
	IndexEntry(std::unique_ptr<AstNode> index, std::unique_ptr<AstNode> value,
			   Token* token_open_bracket, Token* token_close_bracket, Token* token_equals)
		: index_(std::move(index))
		, value_(std::move(value))
		, token_open_bracket_(token_open_bracket)
		, token_close_bracket_(token_close_bracket)
		, token_equals_(token_equals)
	{}
	~IndexEntry() override = default;
	TableEntryType GetType() const noexcept override { return TableEntryType::Index; }

	std::unique_ptr<AstNode> index_;                 // 索引表达式
	std::unique_ptr<AstNode> value_;                 // 值表达式
	Token*                   token_open_bracket_;    // '['
	Token*                   token_close_bracket_;   // ']'
	Token*                   token_equals_;          // '='
};

class FieldEntry : public TableEntry
{
public:
	FieldEntry(Token* token_field, std::unique_ptr<AstNode> value, Token* token_equals)
		: field_(token_field)
		, value_(std::move(value))
		, token_equals_(token_equals)
	{}
	~FieldEntry() override = default;
	TableEntryType GetType() const noexcept override { return TableEntryType::Field; }

	Token*                   field_;          // 字段名
	std::unique_ptr<AstNode> value_;          // 值表达式
	Token*                   token_equals_;   // '='
};

class ValueEntry : public TableEntry
{
public:
	ValueEntry(std::unique_ptr<AstNode> value)
		: value_(std::move(value))
	{}
	~ValueEntry() override = default;
	TableEntryType GetType() const noexcept override { return TableEntryType::Value; }

	std::unique_ptr<AstNode> value_;   // 值表达式
};


/**
 * @brief 表字面量
 *
 */
class TableLiteral : public AstNode
{
public:
	TableLiteral(std::vector<std::unique_ptr<TableEntry>> entry_list,
				 std::vector<Token*> token_seperator_list, Token* token_open_brace,
				 Token* token_close_brace)
		: entry_list_(std::move(entry_list))
		, token_separator_list_(std::move(token_seperator_list))
		, token_open_brace_(token_open_brace)
		, token_close_brace_(token_close_brace)
	{}

	~TableLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_open_brace_; }
	Token*      GetLastToken() const noexcept override { return token_close_brace_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::TableLiteral; }

	std::vector<std::unique_ptr<TableEntry>> entry_list_;
	std::vector<Token*>                      token_separator_list_;
	Token*                                   token_open_brace_;
	Token*                                   token_close_brace_;
};

class FunctionLiteral : public AstNode
{
public:
	FunctionLiteral(std::vector<Token*> arg_list, std::unique_ptr<AstNode> body,
					Token* token_function, Token* token_open_paren,
					std::vector<Token*> token_arg_comma_list, Token* token_close_paren,
					Token* token_end)
		: arg_list_(std::move(arg_list))
		, body_(std::move(body))
		, token_function_(token_function)
		, token_open_paren_(token_open_paren)
		, token_arg_comma_list_(std::move(token_arg_comma_list))
		, token_close_paren_(token_close_paren)
		, token_end_(token_end)
	{}
	~FunctionLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_function_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::FunctionLiteral; }

	std::vector<Token*>      arg_list_;
	std::unique_ptr<AstNode> body_;
	Token*                   token_function_;
	Token*                   token_open_paren_;
	std::vector<Token*>      token_arg_comma_list_;
	Token*                   token_close_paren_;
	Token*                   token_end_;
};


class FunctionStat : public AstNode
{
public:
	FunctionStat(std::vector<Token*> name_chain, std::vector<Token*> arg_list,
				 std::unique_ptr<AstNode> body, Token* token_function,
				 std::vector<Token*> token_name_chain_separator, Token* token_open_paren,
				 std::vector<Token*> token_arg_comma_list, Token* token_close_paren,
				 Token* token_end)
		: name_chain_(std::move(name_chain))
		, arg_list_(std::move(arg_list))
		, body_(std::move(body))
		, token_function_(token_function)
		, token_name_chain_separator_(std::move(token_name_chain_separator))
		, token_open_paren_(token_open_paren)
		, token_arg_comma_list_(std::move(token_arg_comma_list))
		, token_close_paren_(token_close_paren)
		, token_end_(token_end)
	{}
	~FunctionStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_function_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::FunctionStat; }

	std::vector<Token*>      name_chain_;
	std::vector<Token*>      arg_list_;
	std::unique_ptr<AstNode> body_;
	Token*                   token_function_;
	std::vector<Token*>      token_name_chain_separator_;
	Token*                   token_open_paren_;
	std::vector<Token*>      token_arg_comma_list_;
	Token*                   token_close_paren_;
	Token*                   token_end_;
};

class ArgCall : public AstNode
{
public:
	ArgCall(std::vector<std::unique_ptr<AstNode>> arg_list, std::vector<Token*> token_comma_list,
			Token* token_open_paren, Token* token_close_paren)
		: arg_list_(std::move(arg_list))
		, token_comma_list_(std::move(token_comma_list))
		, token_open_paren_(token_open_paren)
		, token_close_paren_(token_close_paren)
	{}
	~ArgCall() override = default;
	Token*      GetFirstToken() const noexcept override { return token_open_paren_; }
	Token*      GetLastToken() const noexcept override { return token_close_paren_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::ArgCall; }

	std::vector<std::unique_ptr<AstNode>> arg_list_;
	std::vector<Token*>                   token_comma_list_;
	Token*                                token_open_paren_;
	Token*                                token_close_paren_;
};

class TableCall : public AstNode
{
public:
	TableCall(std::unique_ptr<AstNode> table_expr)
		: table_expr_(std::move(table_expr))
	{}
	~TableCall() override = default;
	Token*      GetFirstToken() const noexcept override { return table_expr_->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return table_expr_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::TableCall; }

	std::unique_ptr<AstNode> table_expr_;
};

class StringCall : public AstNode
{
public:
	StringCall(Token* token)
		: token_(token)
	{}
	~StringCall() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::StringCall; }

	Token* token_;
};

class FieldExpr : public AstNode
{
public:
	FieldExpr(std::unique_ptr<AstNode> base, Token* field, Token* token_dot)
		: base_(std::move(base))
		, field_(field)
		, token_dot_(token_dot)
	{}
	~FieldExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return base_->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return field_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::FieldExpr; }

	std::unique_ptr<AstNode> base_;
	Token*                   field_;
	Token*                   token_dot_;
};

class MethodExpr : public AstNode
{
public:
	MethodExpr(std::unique_ptr<AstNode> base, Token* method,
			   std::unique_ptr<AstNode> function_arguments, Token* token_colon)
		: base_(std::move(base))
		, method_(method)
		, function_arguments_(std::move(function_arguments))
		, token_colon_(token_colon)
	{}
	~MethodExpr() override = default;
	Token* GetFirstToken() const noexcept override { return base_->GetFirstToken(); }
	Token* GetLastToken() const noexcept override { return function_arguments_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::MethodExpr; }

	std::unique_ptr<AstNode> base_;
	Token*                   method_;
	std::unique_ptr<AstNode> function_arguments_;
	Token*                   token_colon_;
};


class IndexExpr : public AstNode
{
public:
	IndexExpr(std::unique_ptr<AstNode> base, std::unique_ptr<AstNode> index,
			  Token* token_open_bracket, Token* token_close_bracket)
		: base_(std::move(base))
		, index_(std::move(index))
		, token_open_bracket_(token_open_bracket)
		, token_close_bracket_(token_close_bracket)
	{}
	~IndexExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return base_->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return token_close_bracket_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::IndexExpr; }

	std::unique_ptr<AstNode> base_;
	std::unique_ptr<AstNode> index_;
	Token*                   token_open_bracket_;
	Token*                   token_close_bracket_;
};

class CallExpr : public AstNode
{
public:
	CallExpr(std::unique_ptr<AstNode> base, std::unique_ptr<AstNode> function_arguments)
		: base_(std::move(base))
		, function_arguments_(std::move(function_arguments))
	{}
	~CallExpr() override = default;
	Token* GetFirstToken() const noexcept override { return base_->GetFirstToken(); }
	Token* GetLastToken() const noexcept override { return function_arguments_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::CallExpr; }

	std::unique_ptr<AstNode> base_;
	std::unique_ptr<AstNode> function_arguments_;
};

class NumberLiteral : public AstNode
{
public:
	NumberLiteral(Token* token)
		: token_(token)
	{}
	~NumberLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::NumberLiteral; }

	Token* token_;
};

class StringLiteral : public AstNode
{
public:
	StringLiteral(Token* token)
		: token_(token)
	{}
	~StringLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::StringLiteral; }

	Token* token_;
};

class NilLiteral : public AstNode
{
public:
	NilLiteral(Token* token)
		: token_(token)
	{}
	~NilLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::NilLiteral; }

	Token* token_;
};

class BooleanLiteral : public AstNode
{
public:
	BooleanLiteral(Token* token)
		: token_(token)
	{}
	~BooleanLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::BooleanLiteral; }

	Token* token_;
};

class VargLiteral : public AstNode
{
public:
	VargLiteral(Token* token)
		: token_(token)
	{}
	~VargLiteral() override = default;
	Token*      GetFirstToken() const noexcept override { return token_; }
	Token*      GetLastToken() const noexcept override { return token_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::VargLiteral; }

	Token* token_;
};

// class UnopExpr : public AstNode
// {
// public:
//     UnopExpr(Token* token_op, std::unique_ptr<AstNode> rhs)
//         : token_op_(token_op)
//         , rhs_(std::move(rhs))
//     {}
//     ~UnopExpr() override = default;
//     Token*      GetFirstToken() const noexcept override { return token_op_; }
//     Token*      GetLastToken() const noexcept override { return rhs_->GetLastToken(); }
//     AstNodeType GetType() const noexcept override { return AstNodeType::UnopExpr; }

//     Token*                   token_op_;
//     std::unique_ptr<AstNode> rhs_;
// };

class NotExpr : public AstNode
{
public:
	NotExpr(Token* token_op, std::unique_ptr<AstNode> rhs)
		: token_op_(token_op)
		, rhs_(std::move(rhs))
	{}
	~NotExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return token_op_; }
	Token*      GetLastToken() const noexcept override { return rhs_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::NotExpr; }

	Token*                   token_op_;
	std::unique_ptr<AstNode> rhs_;
};

class NegativeExpr : public AstNode
{
public:
	NegativeExpr(Token* token_op, std::unique_ptr<AstNode> rhs)
		: token_op_(token_op)
		, rhs_(std::move(rhs))
	{}
	~NegativeExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return token_op_; }
	Token*      GetLastToken() const noexcept override { return rhs_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::NegativeExpr; }

	Token*                   token_op_;
	std::unique_ptr<AstNode> rhs_;
};

class LengthExpr : public AstNode
{
public:
	LengthExpr(Token* token_op, std::unique_ptr<AstNode> rhs)
		: token_op_(token_op)
		, rhs_(std::move(rhs))
	{}
	~LengthExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return token_op_; }
	Token*      GetLastToken() const noexcept override { return rhs_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::LengthExpr; }

	Token*                   token_op_;
	std::unique_ptr<AstNode> rhs_;
};


class BinopExpr : public AstNode
{
public:
	BinopExpr(std::unique_ptr<AstNode> lhs, Token* token_op, std::unique_ptr<AstNode> rhs)
		: lhs_(std::move(lhs))
		, rhs_(std::move(rhs))
		, token_op_(token_op)
	{}
	~BinopExpr() override = default;
	Token*      GetFirstToken() const noexcept override { return lhs_->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return rhs_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::BinopExpr; }

	std::unique_ptr<AstNode> lhs_;
	std::unique_ptr<AstNode> rhs_;
	Token*                   token_op_;
};

class CallExprStat : public AstNode
{
public:
	CallExprStat(std::unique_ptr<AstNode> expression)
		: expression_(std::move(expression))
	{}
	~CallExprStat() override = default;
	Token*      GetFirstToken() const noexcept override { return expression_->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return expression_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::CallExprStat; }

	std::unique_ptr<AstNode> expression_;
};

class AssignmentStat : public AstNode
{
public:
	AssignmentStat(std::vector<std::unique_ptr<AstNode>> lhs,
				   std::vector<std::unique_ptr<AstNode>> rhs, Token* token_equals,
				   std::vector<Token*> token_lhs_separator_list,
				   std::vector<Token*> token_rhs_separator_list)
		: lhs_(std::move(lhs))
		, rhs_(std::move(rhs))
		, token_equals_(token_equals)
		, token_lhs_separator_list_(std::move(token_lhs_separator_list))
		, token_rhs_separator_list_(std::move(token_rhs_separator_list))
	{}
	~AssignmentStat() override = default;
	Token*      GetFirstToken() const noexcept override { return lhs_.front()->GetFirstToken(); }
	Token*      GetLastToken() const noexcept override { return rhs_.back()->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::AssignmentStat; }

	std::vector<std::unique_ptr<AstNode>> lhs_;
	std::vector<std::unique_ptr<AstNode>> rhs_;
	Token*                                token_equals_;
	std::vector<Token*>                   token_lhs_separator_list_;
	std::vector<Token*>                   token_rhs_separator_list_;
};

class GeneralElseClause
{
public:
	virtual ~GeneralElseClause()                     = default;
	virtual AstNode*         GetCondition() noexcept = 0;
	virtual Token*           GetTokenThen() noexcept = 0;
	std::unique_ptr<AstNode> body_;
	Token*                   token_;
};

class ElseIfClause : public GeneralElseClause
{
public:
	ElseIfClause(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body,
				 Token* token_elseif, Token* token_then)
		: condition_(std::move(condition))
		, token_then_(token_then)
	{
		body_  = std::move(body);
		token_ = token_elseif;
	}
	~ElseIfClause() override = default;
	AstNode* GetCondition() noexcept override { return condition_.get(); }
	Token*   GetTokenThen() noexcept override { return token_then_; }

	std::unique_ptr<AstNode> condition_;
	// std::unique_ptr<AstNode> body_;
	// Token*                   token_;
	Token* token_then_;
};

class ElseClause : public GeneralElseClause
{
public:
	ElseClause(std::unique_ptr<AstNode> body, Token* token_else)
	{
		body_  = std::move(body);
		token_ = token_else;
	}
	~ElseClause() override = default;
	AstNode* GetCondition() noexcept override { return nullptr; }
	Token*   GetTokenThen() noexcept override { return nullptr; }

	// std::unique_ptr<AstNode> body_;
	// Token*                   token_;
};

class IfStat : public AstNode
{
public:
	IfStat(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body,
		   std::vector<std::unique_ptr<GeneralElseClause>> else_clause_list, Token* token_if,
		   Token* token_then, Token* token_end)
		: condition_(std::move(condition))
		, body_(std::move(body))
		, else_clause_list_(std::move(else_clause_list))
		, token_if_(token_if)
		, token_then_(token_then)
		, token_end_(token_end)
	{}
	~IfStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_if_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::IfStat; }

	std::unique_ptr<AstNode>                        condition_;
	std::unique_ptr<AstNode>                        body_;
	std::vector<std::unique_ptr<GeneralElseClause>> else_clause_list_;
	Token*                                          token_if_;
	Token*                                          token_then_;
	Token*                                          token_end_;
};

class DoStat : public AstNode
{
public:
	DoStat(Token* token_do, Token* token_end, std::unique_ptr<AstNode> body)
		: token_do_(token_do)
		, token_end_(token_end)
		, body_(std::move(body))
	{}
	~DoStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_do_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::DoStat; }

	Token*                   token_do_;
	Token*                   token_end_;
	std::unique_ptr<AstNode> body_;
};

class WhileStat : public AstNode
{
public:
	WhileStat(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body, Token* token_while,
			  Token* token_do, Token* token_end)
		: condition_(std::move(condition))
		, body_(std::move(body))
		, token_while_(token_while)
		, token_do_(token_do)
		, token_end_(token_end)
	{}
	~WhileStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_while_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::WhileStat; }

	std::unique_ptr<AstNode> condition_;
	std::unique_ptr<AstNode> body_;
	Token*                   token_while_;
	Token*                   token_do_;
	Token*                   token_end_;
};

class NumericForStat : public AstNode
{
public:
	NumericForStat(std::vector<Token*> var_list, std::vector<std::unique_ptr<AstNode>> range_list,
				   std::unique_ptr<AstNode> body, Token* token_for,
				   std::vector<Token*> token_var_comma_list, Token* token_equals,
				   std::vector<Token*> token_range_separator_list, Token* token_do,
				   Token* token_end)
		: var_list_(std::move(var_list))
		, range_list_(std::move(range_list))
		, body_(std::move(body))
		, token_for_(token_for)
		, token_var_comma_list_(std::move(token_var_comma_list))
		, token_equals_(token_equals)
		, token_range_comma_list_(std::move(token_range_separator_list))
		, token_do_(token_do)
		, token_end_(token_end)
	{}
	~NumericForStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_for_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::NumericForStat; }

	std::vector<Token*>                   var_list_;
	std::vector<std::unique_ptr<AstNode>> range_list_;
	std::unique_ptr<AstNode>              body_;
	Token*                                token_for_;
	std::vector<Token*>                   token_var_comma_list_;
	Token*                                token_equals_;
	std::vector<Token*>                   token_range_comma_list_;
	Token*                                token_do_;
	Token*                                token_end_;
};

class GenericForStat : public AstNode
{
public:
	GenericForStat(std::vector<Token*>                   var_list,
				   std::vector<std::unique_ptr<AstNode>> iterator_list,
				   std::unique_ptr<AstNode> body, Token* token_for,
				   std::vector<Token*> token_var_comma_list, Token* token_in,
				   std::vector<Token*> token_generator_comma_list, Token* token_do,
				   Token* token_end)
		: var_list_(std::move(var_list))
		, generator_list_(std::move(iterator_list))
		, body_(std::move(body))
		, token_for_(token_for)
		, token_var_comma_list_(std::move(token_var_comma_list))
		, token_in_(token_in)
		, token_generator_comma_list_(std::move(token_generator_comma_list))
		, token_do_(token_do)
		, token_end_(token_end)
	{}
	~GenericForStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_for_; }
	Token*      GetLastToken() const noexcept override { return token_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::GenericForStat; }

	std::vector<Token*>                   var_list_;
	std::vector<std::unique_ptr<AstNode>> generator_list_;
	std::unique_ptr<AstNode>              body_;
	Token*                                token_for_;
	std::vector<Token*>                   token_var_comma_list_;
	Token*                                token_in_;
	std::vector<Token*>                   token_generator_comma_list_;
	Token*                                token_do_;
	Token*                                token_end_;
};

class RepeatStat : public AstNode
{
public:
	RepeatStat(std::unique_ptr<AstNode> body, std::unique_ptr<AstNode> condition,
			   Token* token_repeat, Token* token_until)
		: body_(std::move(body))
		, condition_(std::move(condition))
		, token_repeat_(token_repeat)
		, token_until_(token_until)
	{}
	~RepeatStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_repeat_; }
	Token*      GetLastToken() const noexcept override { return condition_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::RepeatStat; }

	std::unique_ptr<AstNode> body_;
	std::unique_ptr<AstNode> condition_;
	Token*                   token_repeat_;
	Token*                   token_until_;
};

class LocalFunctionStat : public AstNode
{
public:
	LocalFunctionStat(std::unique_ptr<AstNode> function_stat, Token* token_local)
		: function_stat_(std::move(function_stat))
		, token_local_(token_local)
	{}
	~LocalFunctionStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_local_; }
	Token*      GetLastToken() const noexcept override { return function_stat_->GetLastToken(); }
	AstNodeType GetType() const noexcept override { return AstNodeType::LocalFunctionStat; }

	std::unique_ptr<AstNode> function_stat_;
	Token*                   token_local_;
};

class LocalVarStat : public AstNode
{
public:
	LocalVarStat(std::vector<Token*> var_list, std::vector<std::unique_ptr<AstNode>> expr_list,
				 Token* token_local, Token* token_equals, std::vector<Token*> token_var_comma_list,
				 std::vector<Token*> expr_comma_list)
		: var_list_(std::move(var_list))
		, expr_list_(std::move(expr_list))
		, token_local_(token_local)
		, token_equals_(token_equals)
		, token_var_comma_list_(std::move(token_var_comma_list))
		, token_expr_comma_list_(std::move(expr_comma_list))
	{}
	~LocalVarStat() override = default;
	Token* GetFirstToken() const noexcept override { return token_local_; }
	Token* GetLastToken() const noexcept override
	{
		if (!expr_list_.empty()) {
			return expr_list_.back()->GetLastToken();
		}
		return var_list_.back();
	}
	AstNodeType GetType() const noexcept override { return AstNodeType::LocalVarStat; }

	std::vector<Token*>                   var_list_;
	std::vector<std::unique_ptr<AstNode>> expr_list_;
	Token*                                token_local_;
	Token*                                token_equals_;   // can be nullptr!
	std::vector<Token*>                   token_var_comma_list_;
	std::vector<Token*>                   token_expr_comma_list_;
};

class ReturnStat : public AstNode
{
public:
	ReturnStat(std::vector<std::unique_ptr<AstNode>> expr_list, Token* token_return,
			   std::vector<Token*> token_comma_list)
		: expr_list_(std::move(expr_list))
		, token_return_(token_return)
		, token_comma_list_(std::move(token_comma_list))
	{}
	~ReturnStat() override = default;
	Token* GetFirstToken() const noexcept override { return token_return_; }
	Token* GetLastToken() const noexcept override
	{
		if (!expr_list_.empty()) {
			return expr_list_.back()->GetLastToken();
		}
		return token_return_;
	}
	AstNodeType GetType() const noexcept override { return AstNodeType::ReturnStat; }

	std::vector<std::unique_ptr<AstNode>> expr_list_;
	Token*                                token_return_;
	std::vector<Token*>                   token_comma_list_;
};

class BreakStat : public AstNode
{
public:
	BreakStat(Token* token_break)
		: token_break_(token_break)
	{}
	~BreakStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_break_; }
	Token*      GetLastToken() const noexcept override { return token_break_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::BreakStat; }

	Token* token_break_;
};

class StatList : public AstNode
{
public:
	StatList(std::vector<std::unique_ptr<AstNode>> statement_list,
			 std::vector<Token*>                   semicolon_list)
		: statement_list(std::move(statement_list))
		, semicolon_list(std::move(semicolon_list))
	{}
	~StatList() override = default;
	Token* GetFirstToken() const noexcept override
	{
		if (!statement_list.empty()) {
			return statement_list.front()->GetFirstToken();
		}
		return nullptr;
	}
	Token* GetLastToken() const noexcept override
	{
		if (!statement_list.empty()) {
			return statement_list.back()->GetLastToken();
		}
		return nullptr;
	}
	AstNodeType GetType() const noexcept override { return AstNodeType::StatList; }

	std::vector<std::unique_ptr<AstNode>> statement_list;
	std::vector<Token*>                   semicolon_list;
};

class GotoStat : public AstNode
{
public:
	GotoStat(Token* token_goto, Token* token_label)
		: token_goto_(token_goto)
		, token_label_(token_label)
	{}
	~GotoStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_goto_; }
	Token*      GetLastToken() const noexcept override { return token_label_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::GotoStat; }

	Token* token_goto_;
	Token* token_label_;
};

class LabelStat : public AstNode
{
public:
	LabelStat(Token* token_label_start, Token* token_label, Token* token_label_end)
		: token_label_start_(token_label_start)
		, token_label_(token_label)
		, token_label_end_(token_label_end)
	{}
	~LabelStat() override = default;
	Token*      GetFirstToken() const noexcept override { return token_label_start_; }
	Token*      GetLastToken() const noexcept override { return token_label_end_; }
	AstNodeType GetType() const noexcept override { return AstNodeType::LabelStat; }

	Token* token_label_start_;
	Token* token_label_;
	Token* token_label_end_;
};
}   // namespace dl