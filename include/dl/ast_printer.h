#pragma once
#include "dl/ast.h"
#include "dl/token.h"
#include <cstring>
#include <ostream>

namespace dl {

enum class AstPrintMode
{
	Compress,
	Auto,
	Manual,
};
template<AstPrintMode mode> class AstPrinter
{
public:
	AstPrinter(std::ostream& out, const std::vector<CommentToken>* comment_tokens = nullptr)
		: out_(out)
		, comment_tokens_(comment_tokens)
		, indent_(0)
	{}

	void PrintAst(const AstNode* ast) noexcept
	{
		print_stat(ast);
		if constexpr (mode != AstPrintMode::Compress) {
			// 注意把文件末尾的注释也打印出来
			while (comment_index_ < comment_tokens_->size()) {
				append('\n');
				indent();
				append(comment_token()->source_);
				++comment_index_;
			}
		}
		flush();
		buffer_pos_ = 0;
	}

private:
	enum class FormatStatGroup
	{
		None,
		Block,
		LocalDecl,
		Label,
		Assign,
		Break,
		Return,
		Call,
		Goto
	};
	void print_token(const Token* token) noexcept
	{
		if constexpr (mode == AstPrintMode::Compress) {
			append(token->source_);
		}
		else if constexpr (mode == AstPrintMode::Auto) {
			line_ = token->line_;
			// 作为一行的开始，应当首先检测头上有没有别的注释，然后再写入 token 内容
			if (line_start_) {
				while (comment_index_ < comment_tokens_->size()) {
					auto comment = comment_token();
					if (line_ > comment->line_) {
						// 这个注释的行号小于当前行，且还没有消费，所以应该输出这个注释
						// print 前 indent 已经做好，所以不用重复 indent()
						// 我们只可能有短注释
						append(comment->source_);
						append('\n');
						++comment_index_;
						indent();
					}
					else {
						break;
					}
				}
				line_start_ = false;
			}
			append(token->source_);
		}
	}
	void print_expr(const AstNode* expr) noexcept
	{
		const auto type = expr->type_;
		if (type == AstNodeType::AddExpr) {
			print_expr(expr->add_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append('+');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" + ");
			}
			print_expr(expr->add_expr_.rhs_);
		}
		else if (type == AstNodeType::SubExpr) {
			print_expr(expr->sub_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append('-');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" - ");
			}
			print_expr(expr->sub_expr_.rhs_);
		}
		else if (type == AstNodeType::MulExpr) {
			print_expr(expr->mul_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append('*');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" * ");
			}
			print_expr(expr->mul_expr_.rhs_);
		}
		else if (type == AstNodeType::DivExpr) {
			print_expr(expr->div_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('/');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" / ");
			}
			print_expr(expr->div_expr_.rhs_);
		}
		else if (type == AstNodeType::ModExpr) {
			print_expr(expr->mod_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('%');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" % ");
			}
			print_expr(expr->mod_expr_.rhs_);
		}
		else if (type == AstNodeType::PowExpr) {
			print_expr(expr->pow_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('^');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" ^ ");
			}
			print_expr(expr->pow_expr_.rhs_);
		}
		else if (type == AstNodeType::ConcatExpr) {
			print_expr(expr->concat_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append("..");
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" .. ");
			}
			print_expr(expr->concat_expr_.rhs_);
		}
		else if (type == AstNodeType::EqExpr) {
			print_expr(expr->eq_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append("==");
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" == ");
			}
			print_expr(expr->eq_expr_.rhs_);
		}
		else if (type == AstNodeType::NeqExpr) {
			print_expr(expr->neq_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append("~=");
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" ~= ");
			}
			print_expr(expr->neq_expr_.rhs_);
		}
		else if (type == AstNodeType::LtExpr) {
			print_expr(expr->lt_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('<');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" < ");
			}
			print_expr(expr->lt_expr_.rhs_);
		}
		else if (type == AstNodeType::LeExpr) {
			print_expr(expr->le_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append("<=");
			}
			else if constexpr (mode == AstPrintMode::Auto) {



				append(" <= ");
			}
			print_expr(expr->le_expr_.rhs_);
		}
		else if (type == AstNodeType::GtExpr) {
			print_expr(expr->gt_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('>');
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" > ");
			}
			print_expr(expr->gt_expr_.rhs_);
		}
		else if (type == AstNodeType::GeExpr) {
			print_expr(expr->ge_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append(">=");
			}
			else if constexpr (mode == AstPrintMode::Auto) {
				append(" >= ");
			}
			print_expr(expr->ge_expr_.rhs_);
		}
		else if (type == AstNodeType::AndExpr) {
			print_expr(expr->and_expr_.lhs_);
			append(" and ");
			print_expr(expr->and_expr_.rhs_);
		}
		else if (type == AstNodeType::OrExpr) {
			print_expr(expr->or_expr_.lhs_);
			append(" or ");
			print_expr(expr->or_expr_.rhs_);
		}
		else if (type == AstNodeType::NotExpr) {
			print_token(expr->first_token_);
			space();
			print_expr(expr->not_expr_.rhs_);
		}
		else if (type == AstNodeType::LengthExpr) {
			print_token(expr->first_token_);
			print_expr(expr->length_expr_.rhs_);
		}
		else if (type == AstNodeType::NegativeExpr) {
			print_token(expr->first_token_);
			print_expr(expr->negative_expr_.rhs_);
		}
		else if (type == AstNodeType::NumberLiteral || type == AstNodeType::StringLiteral ||
				 type == AstNodeType::NilLiteral || type == AstNodeType::BooleanLiteral ||
				 type == AstNodeType::VargLiteral) {
			print_token(expr->first_token_);
		}
		else if (type == AstNodeType::FieldExpr) {

			print_expr(expr->field_expr_.base_);
			append('.');
			print_token(expr->field_expr_.field_);
		}
		else if (type == AstNodeType::IndexExpr) {
			print_expr(expr->index_expr_.base_);
			append('[');
			print_expr(expr->index_expr_.index_);
			append(']');
		}
		else if (type == AstNodeType::MethodExpr) {
			print_expr(expr->method_expr_.base_);
			append(':');
			print_token(expr->method_expr_.method_);

			const auto function_args = expr->method_expr_.function_arguments_;
			const auto call_type     = function_args->type_;
			if (call_type == AstNodeType::StringCall) {
				print_token(function_args->first_token_);
			}
			else if (call_type == AstNodeType::ArgCall) {
				auto& arg_call = function_args->arg_call_;
				append('(');
				for (size_t i = 0; i < arg_call.arg_list_.size(); ++i) {
					print_expr(arg_call.arg_list_[i]);
					if (i < arg_call.arg_list_.size() - 1) {
						append(',');
						if constexpr (mode != AstPrintMode::Compress) {
							space();
						}
					}
				}
				append(')');
			}
			else if (call_type == AstNodeType::TableCall) {
				print_expr(expr->table_call_.table_expr_);
			}
		}
		else if (type == AstNodeType::CallExpr) {
			auto& node = expr->call_expr_;
			print_expr(node.base_);

			const auto function_args = node.function_arguments_;
			const auto call_type     = function_args->type_;
			if (call_type == AstNodeType::StringCall) {
				print_token(function_args->first_token_);
			}
			else if (call_type == AstNodeType::ArgCall) {
				auto& arg_call = function_args->arg_call_;
				append('(');
				for (size_t i = 0; i < arg_call.arg_list_.size(); ++i) {
					print_expr(arg_call.arg_list_[i]);
					if (i < arg_call.arg_list_.size() - 1) {
						append(',');
						if constexpr (mode != AstPrintMode::Compress) {
							space();
						}
					}
				}
				append(')');
			}
			else if (call_type == AstNodeType::TableCall) {
				print_expr(function_args->table_call_.table_expr_);
			}
		}
		else if (type == AstNodeType::FunctionLiteral) {
			auto& node = expr->function_literal_;
			print_token(expr->first_token_);
			append('(');
			for (size_t i = 0; i < node.arg_list_.size(); ++i) {
				print_token(node.arg_list_[i]);
				if (i < node.arg_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			append(')');
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (type == AstNodeType::VariableExpr) {
			print_token(expr->first_token_);
		}
		else if (type == AstNodeType::ParenExpr) {
			auto& node = expr->paren_expr_;
			print_token(expr->first_token_);
			print_expr(node.expression_);
			append(')');
		}
		else if (type == AstNodeType::TableLiteral) {
			auto& node = expr->table_literal_;
			print_token(expr->first_token_);
			if (!node.entry_list_.empty()) {
				if constexpr (mode == AstPrintMode::Compress) {
					for (size_t i = 0; i < node.entry_list_.size(); ++i) {
						auto       entry      = node.entry_list_[i];
						const auto entry_type = entry.type_;
						if (entry_type == AstNode::TableEntryType::Field) {
							auto& field_entry = entry.field_entry_;
							print_token(field_entry.field_);

							append('=');

							print_expr(field_entry.value_);
						}
						else if (entry_type == AstNode::TableEntryType::Index) {
							auto& index_entry = entry.index_entry_;
							append('[');
							print_expr(index_entry.index_);
							append(']');
							append('=');
							print_expr(index_entry.value_);
						}
						else if (entry_type == AstNode::TableEntryType::Value) {
							auto& value_entry = entry.value_entry_;
							print_expr(value_entry.value_);
						}
						// Other entry type UNREACHABLE
						if (i < node.entry_list_.size() - 1) {
							append(',');
						}
					}
				}
				else if constexpr (mode == AstPrintMode::Auto) {
					// 对于纯 value entry 且较短的表，尝试一行输出
					bool one_line = true;
					if (node.entry_list_.size() > 10) {
						one_line = false;
					}
					else {
						for (size_t i = 0; i < node.entry_list_.size(); ++i) {
							const auto entry_type = node.entry_list_[i].type_;
							if (entry_type != AstNode::TableEntryType::Value) {
								one_line = false;
								break;
							}
						}
					}

					if (one_line) {
						// 单行输出
						for (size_t i = 0; i < node.entry_list_.size(); ++i) {
							auto  entry       = node.entry_list_[i];
							auto& value_entry = entry.value_entry_;
							print_expr(value_entry.value_);
							// Other entry type UNREACHABLE
							if (i < node.entry_list_.size() - 1) {
								append(", ");
							}
						}
					}
					else {
						breakline();
						inc_indent();
						for (size_t i = 0; i < node.entry_list_.size(); ++i) {
							auto       entry      = node.entry_list_[i];
							const auto entry_type = entry.type_;
							indent();
							if (entry_type == AstNode::TableEntryType::Field) {
								auto& field_entry = entry.field_entry_;
								print_token(field_entry.field_);
								append(" = ");
								print_expr(field_entry.value_);
							}
							else if (entry_type == AstNode::TableEntryType::Index) {
								auto& index_entry = entry.index_entry_;
								append('[');
								print_expr(index_entry.index_);
								append("] = ");
								print_expr(index_entry.value_);
							}
							else if (entry_type == AstNode::TableEntryType::Value) {
								auto& value_entry = entry.value_entry_;
								print_expr(value_entry.value_);
							}
							// Other entry type UNREACHABLE
							if (i < node.entry_list_.size() - 1) {
								append(',');
							}
							breakline();
						}
						dec_indent();
						indent();
					}
				}
			}
			append('}');
		}
	}
	void print_stat(const AstNode* stat) noexcept
	{
		if (stat->type_ == AstNodeType::StatList) {
			auto& node = stat->stat_list_;
			for (auto& stat : node.statement_list_) {
				print_stat(stat);
			}
			return;
		}

		if constexpr (mode != AstPrintMode::Compress) {
			do_format_stat_group_rules(stat);
			indent();
		}

		if (stat->type_ == AstNodeType::BreakStat) {
			print_token(stat->first_token_);
		}
		else if (stat->type_ == AstNodeType::ReturnStat) {
			auto& node = stat->return_stat_;
			print_token(stat->first_token_);
			if (!node.expr_list_.empty()) {
				space();
				for (size_t i = 0; i < node.expr_list_.size(); ++i) {
					print_expr(node.expr_list_[i]);
					if (i < node.expr_list_.size() - 1) {
						append(", ");
					}
				}
			}
		}
		else if (stat->type_ == AstNodeType::LocalVarStat) {
			auto& node = stat->local_var_stat_;
			print_token(stat->first_token_);
			space();
			for (size_t i = 0; i < node.var_list_.size(); ++i) {
				print_token(node.var_list_[i]);
				if (i < node.var_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			if (node.expr_list_.size() > 0) {
				if constexpr (mode != AstPrintMode::Compress) {
					append(" = ");
				}
				else {
					append('=');
				}
				for (size_t i = 0; i < node.expr_list_.size(); ++i) {
					print_expr(node.expr_list_[i]);
					if (i < node.expr_list_.size() - 1) {
						append(',');
						if constexpr (mode != AstPrintMode::Compress) {
							space();
						}
					}
				}
			}
		}
		else if (stat->type_ == AstNodeType::LocalFunctionStat) {
			auto& node = stat->local_function_stat_;
			print_token(stat->first_token_);
			space();
			auto function_node = node.function_stat_;
			print_token(function_node->first_token_);
			space();
			auto& function_stat = function_node->function_stat_;
			print_token(function_stat.name_chain_[0]);
			append('(');
			for (size_t i = 0; i < function_stat.arg_list_.size(); ++i) {
				print_token(function_stat.arg_list_[i]);
				if (i < function_stat.arg_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			append(')');
			enter_group();
			print_stat(function_stat.body_);
			exit_group();
			print_token(function_stat.end_token_);
		}
		else if (stat->type_ == AstNodeType::FunctionStat) {
			auto& function_stat = stat->function_stat_;
			print_token(stat->first_token_);
			space();
			for (size_t i = 0; i < function_stat.name_chain_.size(); ++i) {
				print_token(function_stat.name_chain_[i]);
				if (i < function_stat.name_chain_.size() - 1) {
					if (function_stat.is_method_ && i == function_stat.name_chain_.size() - 2) {
						append(':');
					}
					else {
						append('.');
					}
				}
			}
			append('(');
			for (size_t i = 0; i < function_stat.arg_list_.size(); ++i) {
				print_token(function_stat.arg_list_[i]);
				if (i < function_stat.arg_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			append(')');
			enter_group();
			print_stat(function_stat.body_);
			exit_group();
			print_token(function_stat.end_token_);
		}
		else if (stat->type_ == AstNodeType::RepeatStat) {
			auto& node = stat->repeat_stat_;
			print_token(stat->first_token_);
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.until_token_);
			space();
			print_expr(node.condition_);
		}
		else if (stat->type_ == AstNodeType::GenericForStat) {
			auto& node = stat->generic_for_stat_;
			print_token(stat->first_token_);
			space();
			for (size_t i = 0; i < node.var_list_.size(); ++i) {
				print_token(node.var_list_[i]);
				if (i < node.var_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			space();
			append("in");
			space();
			for (size_t i = 0; i < node.generator_list_.size(); ++i) {
				print_expr(node.generator_list_[i]);
				if (i < node.generator_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			space();
			append("do");
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::NumericForStat) {
			auto& node = stat->numeric_for_stat_;
			print_token(stat->first_token_);
			space();
			for (size_t i = 0; i < node.var_list_.size(); ++i) {
				print_token(node.var_list_[i]);
				if (i < node.var_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			if constexpr (mode != AstPrintMode::Compress) {
				append(" = ");
			}
			else {
				append('=');
			}
			for (size_t i = 0; i < node.range_list_.size(); ++i) {
				print_expr(node.range_list_[i]);
				if (i < node.range_list_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			space();
			append("do");
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::WhileStat) {
			auto& node = stat->while_stat_;
			print_token(stat->first_token_);
			space();
			print_expr(node.condition_);
			append(" do");
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::DoStat) {
			auto& node = stat->do_stat_;
			print_token(stat->first_token_);
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::IfStat) {
			auto& node = stat->if_stat_;
			print_token(stat->first_token_);
			space();
			print_expr(node.condition_);
			append(" then");
			enter_group();
			print_stat(node.body_);
			exit_group();
			for (size_t i = 0; i < node.else_clauses_.size(); ++i) {
				auto& clause = node.else_clauses_[i];
				print_token(clause.else_token_);
				if (clause.type_ == AstNode::ElseClauseType::ElseIfClause) {
					space();
					print_expr(clause.else_if_clause_.condition_);
					append(" then");
				}
				enter_group();
				print_stat(clause.body_);
				exit_group();
			}
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::CallExprStat) {
			print_expr(stat->call_expr_stat_.expression_);
		}
		else if (stat->type_ == AstNodeType::AssignmentStat) {
			auto& node = stat->assignment_stat_;
			for (size_t i = 0; i < node.lhs_.size(); ++i) {
				print_expr(node.lhs_[i]);
				if (i < node.lhs_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
			if constexpr (mode != AstPrintMode::Compress) {
				append(" = ");
			}
			else {
				append('=');
			}
			for (size_t i = 0; i < node.rhs_.size(); ++i) {
				print_expr(node.rhs_[i]);
				if (i < node.rhs_.size() - 1) {
					append(',');
					if constexpr (mode != AstPrintMode::Compress) {
						space();
					}
				}
			}
		}
		else if (stat->type_ == AstNodeType::GotoStat) {
			auto& node = stat->goto_stat_;
			print_token(stat->first_token_);
			space();
			print_token(node.label_);
		}
		else if (stat->type_ == AstNodeType::LabelStat) {
			auto& node = stat->label_stat_;
			print_token(stat->first_token_);
			print_token(node.label_);
			append("::");
		}
		breakline();
		if constexpr (mode != AstPrintMode::Compress) {
			set_format_stat_group(get_format_stat_group(stat));
		}
		return;
	}
	void set_format_stat_group(FormatStatGroup group) noexcept
	{
		if constexpr (mode != AstPrintMode::Compress) {
			last_format_stat_group_ = group;
		}
	}
	void do_format_stat_group_rules(const AstNode* stat) noexcept
	{
		if constexpr (mode != AstPrintMode::Compress) {
			// 空白组不处理，即某个块的开始
			if (last_format_stat_group_ == FormatStatGroup::None) {
				return;
			}
			auto stat_group = get_format_stat_group(stat);

			// 块级语句之间也仍然要换行
			if (stat_group == FormatStatGroup::Block) {
				append('\n');
				return;
			}

			// 如果组不同，则换行
			if (stat_group != last_format_stat_group_) {
				// 不同组，插入空行
				append('\n');
			}
		}
	}
	FormatStatGroup get_format_stat_group(const AstNode* stat) const noexcept
	{
		switch (stat->type_) {
		case AstNodeType::BreakStat: return FormatStatGroup::Break;
		case AstNodeType::ReturnStat: return FormatStatGroup::Return;
		case AstNodeType::LocalVarStat: return FormatStatGroup::LocalDecl;
		case AstNodeType::LocalFunctionStat:
		case AstNodeType::FunctionStat:
		case AstNodeType::RepeatStat:
		case AstNodeType::GenericForStat:
		case AstNodeType::NumericForStat:
		case AstNodeType::WhileStat:
		case AstNodeType::DoStat:
		case AstNodeType::IfStat: return FormatStatGroup::Block;
		case AstNodeType::CallExprStat: return FormatStatGroup::Call;
		case AstNodeType::AssignmentStat: return FormatStatGroup::Assign;
		case AstNodeType::GotoStat: return FormatStatGroup::Goto;
		case AstNodeType::LabelStat: return FormatStatGroup::Label;
		default: return FormatStatGroup::None;
		}
	}
	const CommentToken* comment_token() const noexcept
	{
		return &(*comment_tokens_)[comment_index_];
	}

	/**
	 * @brief Simply I don't think indent would exceed 32 in normal use
	 *
	 */
	void indent() noexcept
	{
		// for (int i = 0; i < indent_; ++i) {
		// append('\t');
		// }
		static constexpr int  MAX_INDENT = 32;
		static constexpr char tabs[MAX_INDENT] =
			"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		append(tabs, indent_);
	}
	void space() noexcept { append(' '); }
	void breakline() noexcept
	{
		if constexpr (mode == AstPrintMode::Compress) {
			append('\n');
		}
		else {
			if (comment_index_ < comment_tokens_->size()) {
				auto comment = comment_token();
				if (line_ == comment->line_) {
					// 发现当前行有注释，在行末追加
					space();
					append(comment->source_);
					++comment_index_;
				}
			}
			append('\n');
			line_start_ = true;
		}
	}

	bool is_block_stat(AstNodeType type) const noexcept
	{
		switch (type) {
		case AstNodeType::LocalFunctionStat:
		case AstNodeType::FunctionStat:
		case AstNodeType::RepeatStat:
		case AstNodeType::GenericForStat:
		case AstNodeType::NumericForStat:
		case AstNodeType::WhileStat:
		case AstNodeType::DoStat:
		case AstNodeType::IfStat: return true;
		default: return false;
		}
	}
	void inc_indent() noexcept
	{
		++indent_;
		is_block_start_ = true;
	}
	void dec_indent() noexcept
	{
		--indent_;
		is_block_start_ = false;
	}

	void enter_group() noexcept
	{
		breakline();
		if constexpr (mode != AstPrintMode::Compress) {
			++indent_;
			set_format_stat_group(FormatStatGroup::None);
		}
	}

	void exit_group() noexcept
	{
		if constexpr (mode != AstPrintMode::Compress) {
			--indent_;
			indent();
		}
	}

	void flush() noexcept
	{
		out_.write(buffer_, buffer_pos_);
		buffer_pos_ = 0;
	}

	/**
	 * @brief Append data to buffer
	 * @note Simply I don't think an append would exceed buffer size in normal use
	 *
	 * @param data
	 * @param size < BUFFERSIZE
	 */
	void append(const char* data, size_t size) noexcept
	{
		if (buffer_pos_ + size > BUFFERSIZE) {
			flush();
		}
		std::memcpy(buffer_ + buffer_pos_, data, size);
		buffer_pos_ += size;
	}

	void append(char c) noexcept
	{
		if (buffer_pos_ + 1 > BUFFERSIZE) {
			flush();
		}
		buffer_[buffer_pos_++] = c;
	}

	void append(const std::string_view& str) noexcept { append(str.data(), str.size()); }

	// 64 KB buffer size
	static constexpr size_t          BUFFERSIZE = 64 * 1024;
	std::ostream&                    out_;
	char                             buffer_[BUFFERSIZE];
	size_t                           buffer_pos_     = 0;
	std::size_t                      line_           = 1;
	std::size_t                      comment_index_  = 0;
	const std::vector<CommentToken>* comment_tokens_ = nullptr;
	int                              indent_;
	FormatStatGroup                  last_format_stat_group_ = FormatStatGroup::None;
	bool                             line_start_             = true;
	bool                             last_is_block_stat_     = false;
	bool                             is_block_start_         = true;
};
}   // namespace dl