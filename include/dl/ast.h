#pragma once

#include "dl/token.h"
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

	// Literal Types Begin
	NumberLiteral,
	StringLiteral,
	NilLiteral,
	BooleanLiteral,
	VargLiteral,
	// Literal Types End

	// BinopExpr Types Begin
	AddExpr,
	SubExpr,
	MulExpr,
	DivExpr,
	PowExpr,
	ModExpr,
	ConcatExpr,
	EqExpr,
	NeqExpr,
	LtExpr,
	LeExpr,
	GtExpr,
	GeExpr,
	AndExpr,
	OrExpr,
	// BinopExpr Types End

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

	// UnopExpr Types Begin
	NotExpr,
	NegativeExpr,
	LengthExpr,
	// UnopExpr Types End
};
class AstNode
{
public:
	/**
	 * @brief Parenthesized Expression
	 *
	 */
	struct ParenExpr
	{
		AstNode* expression_;
	};

	struct VariableExpr
	{
		Token* token_;
	};
	enum class TableEntryType
	{
		Index,
		Field,
		Value
	};

	struct TableEntry
	{
		struct IndexEntry
		{
			AstNode* index_;
			AstNode* value_;
		};

		struct FieldEntry
		{
			Token*   field_;
			AstNode* value_;
		};

		struct ValueEntry
		{
			AstNode* value_;
		};
		union
		{
			IndexEntry index_entry_;
			FieldEntry field_entry_;
			ValueEntry value_entry_;
		};
		TableEntryType type_;
		TableEntry(IndexEntry&& v)
			: type_(TableEntryType::Index)
		{
			new (&index_entry_) IndexEntry(std::move(v));
		}
		TableEntry(FieldEntry&& v)
			: type_(TableEntryType::Field)
		{
			new (&field_entry_) FieldEntry(std::move(v));
		}
		TableEntry(ValueEntry&& v)
			: type_(TableEntryType::Value)
		{
			new (&value_entry_) ValueEntry(std::move(v));
		}
	};

	struct TableLiteral
	{
		std::vector<TableEntry> entry_list_;
        Token* end_token_;
	};

	struct FunctionLiteral
	{
		std::vector<Token*>* arg_list_;
		AstNode*            body_;
		Token*              end_token_;
	};

	struct FunctionStat
	{
		std::vector<Token*>* name_chain_;
		std::vector<Token*>* arg_list_;
		AstNode*            body_;
		Token*              end_token_;
		bool                is_method_;
	};

	struct ArgCall
	{
		std::vector<AstNode*>* arg_list_;
	};

	struct TableCall
	{
		AstNode* table_expr_;
	};

	struct StringCall
	{};

	struct FieldExpr
	{
		AstNode* base_;
		Token*   field_;
	};

	struct MethodExpr
	{
		AstNode* base_;
		Token*   method_;
		AstNode* function_arguments_;
	};

	struct IndexExpr
	{
		AstNode* base_;
		AstNode* index_;
	};

	struct CallExpr
	{
		AstNode* base_;
		AstNode* function_arguments_;
	};

	struct NumberLiteral
	{};

	struct StringLiteral
	{};

	struct NilLiteral
	{};

	struct BooleanLiteral
	{};

	struct VargLiteral
	{};

	struct NotExpr
	{
		AstNode* rhs_;
	};

	struct NegativeExpr
	{
		AstNode* rhs_;
	};

	struct LengthExpr
	{
		AstNode* rhs_;
	};

	struct AddExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct SubExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct MulExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct DivExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct PowExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct ModExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct ConcatExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct EqExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct NeqExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct LtExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct LeExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct GtExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct GeExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct AndExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct OrExpr
	{
		AstNode* lhs_;
		AstNode* rhs_;
	};

	struct CallExprStat
	{
		AstNode* expression_;
	};

	struct AssignmentStat
	{
		std::vector<AstNode*>* lhs_;
		std::vector<AstNode*>* rhs_;
	};
	enum class ElseClauseType
	{
		ElseIfClause,
		ElseClause
	};
	struct IfStat
	{
		struct ElseClause
		{};
		struct ElseIfClause
		{
			AstNode* condition_;
		};
		struct GeneralElseClause
		{
			union
			{
				ElseIfClause else_if_clause_;
				ElseClause   else_clause_;
			};
			Token*         else_token_;
			AstNode*       body_;
			ElseClauseType type_;
			GeneralElseClause(ElseIfClause&& v, AstNode* body, Token* else_token)
				: else_token_(else_token)
				, body_(body)
				, type_(ElseClauseType::ElseIfClause)
			{
				new (&else_if_clause_) ElseIfClause(std::move(v));
			}
			GeneralElseClause(ElseClause&& v, AstNode* body, Token* else_token)
				: else_token_(else_token)
				, body_(body)
				, type_(ElseClauseType::ElseClause)
			{
				new (&else_clause_) ElseClause(std::move(v));
			}
		};
		AstNode*                       condition_;
		AstNode*                       body_;
		std::vector<GeneralElseClause>* else_clauses_;
		Token*                         end_token_;
	};

	struct DoStat
	{
		AstNode* body_;
		Token*   end_token_;
	};

	struct WhileStat
	{
		AstNode* condition_;
		AstNode* body_;
		Token*   end_token_;
	};

	struct NumericForStat
	{
		std::vector<Token*>*   var_list_;
		std::vector<AstNode*>* range_list_;
		AstNode*              body_;
		Token*                end_token_;
	};

	struct GenericForStat
	{
		std::vector<Token*>*   var_list_;
		std::vector<AstNode*>* generator_list_;
		AstNode*              body_;
		Token*                end_token_;
	};

	struct RepeatStat
	{
		AstNode* body_;
		Token*   until_token_;
		AstNode* condition_;
	};

	struct LocalFunctionStat
	{
		AstNode* function_stat_;
	};

	struct LocalVarStat
	{
		std::vector<Token*>*   var_list_;
		std::vector<AstNode*>* expr_list_;
	};

	struct ReturnStat
	{
		std::vector<AstNode*>* expr_list_;
	};

	struct BreakStat
	{};
	struct StatList
	{
		std::vector<AstNode*>* statement_list_;
	};

	struct GotoStat
	{
		Token* label_;
	};

	struct LabelStat
	{
		Token* label_;
	};

public:
	union
	{
		ParenExpr         paren_expr_;
		VariableExpr      variable_expr_;
		TableLiteral      table_literal_;
		FunctionLiteral   function_literal_;
		FunctionStat      function_stat_;
		ArgCall           arg_call_;
		TableCall         table_call_;
		StringCall        string_call_;
		FieldExpr         field_expr_;
		MethodExpr        method_expr_;
		IndexExpr         index_expr_;
		CallExpr          call_expr_;
		NumberLiteral     number_literal_;
		StringLiteral     string_literal_;
		NilLiteral        nil_literal_;
		BooleanLiteral    boolean_literal_;
		VargLiteral       varg_literal_;
		NotExpr           not_expr_;
		NegativeExpr      negative_expr_;
		LengthExpr        length_expr_;
		AddExpr           add_expr_;
		SubExpr           sub_expr_;
		MulExpr           mul_expr_;
		DivExpr           div_expr_;
		PowExpr           pow_expr_;
		ModExpr           mod_expr_;
		ConcatExpr        concat_expr_;
		EqExpr            eq_expr_;
		NeqExpr           neq_expr_;
		LtExpr            lt_expr_;
		LeExpr            le_expr_;
		GtExpr            gt_expr_;
		GeExpr            ge_expr_;
		AndExpr           and_expr_;
		OrExpr            or_expr_;
		CallExprStat      call_expr_stat_;
		AssignmentStat    assignment_stat_;
		IfStat            if_stat_;
		DoStat            do_stat_;
		WhileStat         while_stat_;
		NumericForStat    numeric_for_stat_;
		GenericForStat    generic_for_stat_;
		RepeatStat        repeat_stat_;
		LocalFunctionStat local_function_stat_;
		LocalVarStat      local_var_stat_;
		ReturnStat        return_stat_;
		BreakStat         break_stat_;
		StatList          stat_list_;
		GotoStat          goto_stat_;
		LabelStat         label_stat_;
	};
	Token*      first_token_;
	AstNodeType type_;
	AstNode(ParenExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::ParenExpr)
	{
		new (&paren_expr_) ParenExpr(std::move(v));
	}
	AstNode(VariableExpr&& v)
		: first_token_(v.token_)
		, type_(AstNodeType::VariableExpr)
	{
		new (&variable_expr_) VariableExpr(std::move(v));
	}
	AstNode(TableLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::TableLiteral)
	{
		new (&table_literal_) TableLiteral(std::move(v));
	}
	AstNode(FunctionLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::FunctionLiteral)
	{
		new (&function_literal_) FunctionLiteral(std::move(v));
	}
	AstNode(FunctionStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::FunctionStat)
	{
		new (&function_stat_) FunctionStat(std::move(v));
	}
	AstNode(ArgCall&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::ArgCall)
	{
		new (&arg_call_) ArgCall(std::move(v));
	}
	AstNode(TableCall&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::TableCall)
	{
		new (&table_call_) TableCall(std::move(v));
	}
	AstNode(StringCall&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::StringCall)
	{
		new (&string_call_) StringCall(std::move(v));
	}
	AstNode(FieldExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::FieldExpr)
	{
		new (&field_expr_) FieldExpr(std::move(v));
	}
	AstNode(MethodExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::MethodExpr)
	{
		new (&method_expr_) MethodExpr(std::move(v));
	}
	AstNode(IndexExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::IndexExpr)
	{
		new (&index_expr_) IndexExpr(std::move(v));
	}
	AstNode(CallExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::CallExpr)
	{
		new (&call_expr_) CallExpr(std::move(v));
	}
	AstNode(NumberLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NumberLiteral)
	{
		new (&number_literal_) NumberLiteral(std::move(v));
	}
	AstNode(StringLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::StringLiteral)
	{
		new (&string_literal_) StringLiteral(std::move(v));
	}
	AstNode(NilLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NilLiteral)
	{
		new (&nil_literal_) NilLiteral(std::move(v));
	}
	AstNode(BooleanLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::BooleanLiteral)
	{
		new (&boolean_literal_) BooleanLiteral(std::move(v));
	}
	AstNode(VargLiteral&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::VargLiteral)
	{
		new (&varg_literal_) VargLiteral(std::move(v));
	}
	AstNode(NotExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NotExpr)
	{
		new (&not_expr_) NotExpr(std::move(v));
	}
	AstNode(NegativeExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NegativeExpr)
	{
		new (&negative_expr_) NegativeExpr(std::move(v));
	}
	AstNode(LengthExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LengthExpr)
	{
		new (&length_expr_) LengthExpr(std::move(v));
	}
	AstNode(AddExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::AddExpr)
	{
		new (&add_expr_) AddExpr(std::move(v));
	}
	AstNode(SubExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::SubExpr)
	{
		new (&sub_expr_) SubExpr(std::move(v));
	}
	AstNode(MulExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::MulExpr)
	{
		new (&mul_expr_) MulExpr(std::move(v));
	}
	AstNode(DivExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::DivExpr)
	{
		new (&div_expr_) DivExpr(std::move(v));
	}
	AstNode(PowExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::PowExpr)
	{
		new (&pow_expr_) PowExpr(std::move(v));
	}
	AstNode(ModExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::ModExpr)
	{
		new (&mod_expr_) ModExpr(std::move(v));
	}
	AstNode(ConcatExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::ConcatExpr)
	{
		new (&concat_expr_) ConcatExpr(std::move(v));
	}
	AstNode(EqExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::EqExpr)
	{
		new (&eq_expr_) EqExpr(std::move(v));
	}
	AstNode(NeqExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NeqExpr)
	{
		new (&neq_expr_) NeqExpr(std::move(v));
	}
	AstNode(LtExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LtExpr)
	{
		new (&lt_expr_) LtExpr(std::move(v));
	}
	AstNode(LeExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LeExpr)
	{
		new (&le_expr_) LeExpr(std::move(v));
	}
	AstNode(GtExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::GtExpr)
	{
		new (&gt_expr_) GtExpr(std::move(v));
	}
	AstNode(GeExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::GeExpr)
	{
		new (&ge_expr_) GeExpr(std::move(v));
	}
	AstNode(AndExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::AndExpr)
	{
		new (&and_expr_) AndExpr(std::move(v));
	}
	AstNode(OrExpr&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::OrExpr)
	{
		new (&or_expr_) OrExpr(std::move(v));
	}
	AstNode(CallExprStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::CallExprStat)
	{
		new (&call_expr_stat_) CallExprStat(std::move(v));
	}
	AstNode(AssignmentStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::AssignmentStat)
	{
		new (&assignment_stat_) AssignmentStat(std::move(v));
	}
	AstNode(IfStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::IfStat)
	{
		new (&if_stat_) IfStat(std::move(v));
	}
	AstNode(DoStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::DoStat)
	{
		new (&do_stat_) DoStat(std::move(v));
	}
	AstNode(WhileStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::WhileStat)
	{
		new (&while_stat_) WhileStat(std::move(v));
	}
	AstNode(NumericForStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::NumericForStat)
	{
		new (&numeric_for_stat_) NumericForStat(std::move(v));
	}
	AstNode(GenericForStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::GenericForStat)
	{
		new (&generic_for_stat_) GenericForStat(std::move(v));
	}
	AstNode(RepeatStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::RepeatStat)
	{
		new (&repeat_stat_) RepeatStat(std::move(v));
	}
	AstNode(LocalFunctionStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LocalFunctionStat)
	{
		new (&local_function_stat_) LocalFunctionStat(std::move(v));
	}
	AstNode(LocalVarStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LocalVarStat)
	{
		new (&local_var_stat_) LocalVarStat(std::move(v));
	}
	AstNode(ReturnStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::ReturnStat)
	{
		new (&return_stat_) ReturnStat(std::move(v));
	}
	AstNode(BreakStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::BreakStat)
	{
		new (&break_stat_) BreakStat(std::move(v));
	}
	AstNode(StatList&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::StatList)
	{
		new (&stat_list_) StatList(std::move(v));
	}
	AstNode(GotoStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::GotoStat)
	{
		new (&goto_stat_) GotoStat(std::move(v));
	}
	AstNode(LabelStat&& v, Token* tok)
		: first_token_(tok)
		, type_(AstNodeType::LabelStat)
	{
		new (&label_stat_) LabelStat(std::move(v));
	}

	~AstNode()
	{
		switch (type_) {
		case AstNodeType::TableLiteral: table_literal_.~TableLiteral(); break;
		// case AstNodeType::FunctionLiteral: function_literal_.~FunctionLiteral(); break;
		// case AstNodeType::FunctionStat: function_stat_.~FunctionStat(); break;
		// case AstNodeType::ArgCall: arg_call_.~ArgCall(); break;
		// case AstNodeType::AssignmentStat: assignment_stat_.~AssignmentStat(); break;
		// case AstNodeType::IfStat: if_stat_.~IfStat(); break;
		// case AstNodeType::NumericForStat: numeric_for_stat_.~NumericForStat(); break;
		// case AstNodeType::GenericForStat: generic_for_stat_.~GenericForStat(); break;
		// case AstNodeType::LocalVarStat: local_var_stat_.~LocalVarStat(); break;
		// case AstNodeType::ReturnStat: return_stat_.~ReturnStat(); break;
		// case AstNodeType::StatList: stat_list_.~StatList(); break;
		default: break;
		}
	}
};

}   // namespace dl