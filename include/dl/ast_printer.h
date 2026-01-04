#pragma once
#include "dl/ast.h"
#include "dl/token.h"
#include <cassert>
#include <cstring>
#include <ostream>
#include <spdlog/spdlog.h>

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
		if constexpr (mode == AstPrintMode::Auto) {
			while (comment_index_ < comment_tokens_->size()) {
				append(comment_token()->source_);
				append('\n');
				++comment_index_;
			}
		}
		else if constexpr (mode == AstPrintMode::Manual) {
			while (comment_index_ < comment_tokens_->size()) {
				auto comment = comment_token();
				switch (comment->type_) {
				case CommentTokenType::ShortComment:
				case CommentTokenType::LongComment:
				{
					append(comment->source_);
					break;
				}
				case CommentTokenType::EmptyLine:
				{
					break;
				}
				}
				append('\n');
				++comment_index_;
			}
		}
		flush();
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
		else {
			line_ = token->line_;
			// 作为一行的开始，应当首先检测头上有没有别的注释，然后再写入 token 内容
			if (line_start_) {
				while (comment_index_ < comment_tokens_->size()) {
					auto comment = comment_token();

					if (line_ > comment->line_) {
						switch (comment->type_) {
						case CommentTokenType::ShortComment:
						// {
						// 	append(comment->source_);
						// 	append('\n');
						// 	++comment_index_;
						// 	indent();
						// 	break;
						// }
						// However I'm too lazy to handle the indent in LongComment case
						case CommentTokenType::LongComment:
						{
							indent();
							append(comment->source_);
							break;
						}
						case CommentTokenType::EmptyLine:
						{
							break;
						}
						}
						append('\n');
						++comment_index_;
					}
					else {
						break;
					}
				}
				indent();
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
			else {
				append(" + ");
			}
			print_expr(expr->add_expr_.rhs_);
		}
		else if (type == AstNodeType::SubExpr) {
			print_expr(expr->sub_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append('-');
			}
			else {
				append(" - ");
			}
			print_expr(expr->sub_expr_.rhs_);
		}
		else if (type == AstNodeType::MulExpr) {
			print_expr(expr->mul_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append('*');
			}
			else {
				append(" * ");
			}
			print_expr(expr->mul_expr_.rhs_);
		}
		else if (type == AstNodeType::DivExpr) {
			print_expr(expr->div_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('/');
			}
			else {
				append(" / ");
			}
			print_expr(expr->div_expr_.rhs_);
		}
		else if (type == AstNodeType::ModExpr) {
			print_expr(expr->mod_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('%');
			}
			else {
				append(" % ");
			}
			print_expr(expr->mod_expr_.rhs_);
		}
		else if (type == AstNodeType::PowExpr) {
			print_expr(expr->pow_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('^');
			}
			else {
				append(" ^ ");
			}
			print_expr(expr->pow_expr_.rhs_);
		}
		else if (type == AstNodeType::ConcatExpr) {
			print_expr(expr->concat_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append("..");
			}
			else {
				append(" .. ");
			}
			print_expr(expr->concat_expr_.rhs_);
		}
		else if (type == AstNodeType::EqExpr) {
			print_expr(expr->eq_expr_.lhs_);
			if constexpr (mode == AstPrintMode::Compress) {
				append("==");
			}
			else {
				append(" == ");
			}
			print_expr(expr->eq_expr_.rhs_);
		}
		else if (type == AstNodeType::NeqExpr) {
			print_expr(expr->neq_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append("~=");
			}
			else {
				append(" ~= ");
			}
			print_expr(expr->neq_expr_.rhs_);
		}
		else if (type == AstNodeType::LtExpr) {
			print_expr(expr->lt_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('<');
			}
			else {
				append(" < ");
			}
			print_expr(expr->lt_expr_.rhs_);
		}
		else if (type == AstNodeType::LeExpr) {
			print_expr(expr->le_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append("<=");
			}
			else {
				append(" <= ");
			}
			print_expr(expr->le_expr_.rhs_);
		}
		else if (type == AstNodeType::GtExpr) {
			print_expr(expr->gt_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append('>');
			}
			else {
				append(" > ");
			}
			print_expr(expr->gt_expr_.rhs_);
		}
		else if (type == AstNodeType::GeExpr) {
			print_expr(expr->ge_expr_.lhs_);

			if constexpr (mode == AstPrintMode::Compress) {
				append(">=");
			}
			else {
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
				const auto& arg_list = *function_args->arg_call_.arg_list_;
				append('(');
				for (size_t i = 0; i < arg_list.size(); ++i) {
					print_expr(arg_list[i]);
					if (i < arg_list.size() - 1) {
						if constexpr (mode != AstPrintMode::Compress) {
							append(", ");
						}
						else {
							append(',');
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
				const auto& arg_list = *function_args->arg_call_.arg_list_;
				append('(');
				for (size_t i = 0; i < arg_list.size(); ++i) {
					print_expr(arg_list[i]);
					if (i < arg_list.size() - 1) {
						if constexpr (mode != AstPrintMode::Compress) {
							append(", ");
						}else{
                            append(',');
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
			const auto& arg_list = *node.arg_list_;
			for (size_t i = 0; i < arg_list.size(); ++i) {
				print_token(arg_list[i]);
				if (i < arg_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
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
							append("]=");
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
				else {
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
							if (entry_type == AstNode::TableEntryType::Field) {
								auto& field_entry = entry.field_entry_;
								print_token(field_entry.field_);
								append(" = ");
								print_expr(field_entry.value_);
							}
							else if (entry_type == AstNode::TableEntryType::Index) {
								auto& index_entry = entry.index_entry_;
                                print_token(index_entry.left_bracket);
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
					}
				}
			}
			print_token(node.end_token_);
		}
	}
	void print_stat(const AstNode* stat) noexcept
	{
		if (stat->type_ == AstNodeType::StatList) {
			const auto& statement_list = *stat->stat_list_.statement_list_;
			for (const auto& stat : statement_list) {
				print_stat(stat);
			}
			return;
		}

		if constexpr (mode == AstPrintMode::Auto) {
			do_format_stat_group_rules(stat);
		}

		if (stat->type_ == AstNodeType::BreakStat) {
			print_token(stat->first_token_);
		}
		else if (stat->type_ == AstNodeType::ReturnStat) {
			auto& node = stat->return_stat_;
			print_token(stat->first_token_);
			const auto& expr_list = *node.expr_list_;
			if (!expr_list.empty()) {
				space();
				for (size_t i = 0; i < expr_list.size(); ++i) {
					print_expr(expr_list[i]);
					if (i < expr_list.size() - 1) {
						append(", ");
					}
				}
			}
		}
		else if (stat->type_ == AstNodeType::LocalVarStat) {
			auto& node = stat->local_var_stat_;
			print_token(stat->first_token_);
			space();
			auto& var_list = *node.var_list_;
			for (size_t i = 0; i < var_list.size(); ++i) {
				print_token(var_list[i]);
				if (i < var_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
                    }
				}
			}
			const auto& expr_list = *node.expr_list_;
			if (expr_list.size() > 0) {
				if constexpr (mode != AstPrintMode::Compress) {
					append(" = ");
				}
				else {
					append('=');
				}
				for (size_t i = 0; i < expr_list.size(); ++i) {
					print_expr(expr_list[i]);
					if (i < expr_list.size() - 1) {
						if constexpr (mode != AstPrintMode::Compress) {
							append(", ");
						}else{
                            append(',');
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
			print_token((*function_stat.name_chain_)[0]);
			append('(');
			const auto& arg_list = *function_stat.arg_list_;
			for (size_t i = 0; i < arg_list.size(); ++i) {
				print_token(arg_list[i]);
				if (i < arg_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
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
			auto& name_chain = *function_stat.name_chain_;
			for (size_t i = 0; i < name_chain.size(); ++i) {
				print_token(name_chain[i]);
				if (i < name_chain.size() - 1) {
					if (function_stat.is_method_ && i == name_chain.size() - 2) {
						append(':');
					}
					else {
						append('.');
					}
				}
			}
			append('(');
			auto& arg_list = *function_stat.arg_list_;
			for (size_t i = 0; i < arg_list.size(); ++i) {
				print_token(arg_list[i]);
				if (i < arg_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
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
			const auto& var_list = *node.var_list_;
			for (size_t i = 0; i < var_list.size(); ++i) {
				print_token(var_list[i]);
				if (i < var_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
                    }
				}
			}
			append(" in ");
			const auto& generator_list = *node.generator_list_;
			for (size_t i = 0; i < generator_list.size(); ++i) {
				print_expr(generator_list[i]);
				if (i < generator_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
                        append(", ");
					}else{
                        append(',');
                    }
				}
			}
			append(" do ");
			enter_group();
			print_stat(node.body_);
			exit_group();
			print_token(node.end_token_);
		}
		else if (stat->type_ == AstNodeType::NumericForStat) {
			auto& node = stat->numeric_for_stat_;
			print_token(stat->first_token_);
			space();
			const auto& var_list = *node.var_list_;
			for (size_t i = 0; i < var_list.size(); ++i) {
				print_token(var_list[i]);
				if (i < var_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
                        append(", ");
					}else{
                        append(',');
                    }
				}
			}
			if constexpr (mode != AstPrintMode::Compress) {
				append(" = ");
			}
			else {
				append('=');
			}
			const auto& range_list = *node.range_list_;
			for (size_t i = 0; i < range_list.size(); ++i) {
				print_expr(range_list[i]);
				if (i < range_list.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
						append(", ");
					}else{
                        append(',');
                    }
				}
			}
			append(" do");
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
			const auto& else_clauses = *node.else_clauses_;
			for (size_t i = 0; i < else_clauses.size(); ++i) {
				auto& clause = else_clauses[i];
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
			auto&       node = stat->assignment_stat_;
			const auto& lhs  = *node.lhs_;
			for (size_t i = 0; i < lhs.size(); ++i) {
				print_expr(lhs[i]);
				if (i < lhs.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
                        append(", ");
					}else{
                        append(',');
                    }
				}
			}
			if constexpr (mode != AstPrintMode::Compress) {
				append(" = ");
			}
			else {
				append('=');
			}
			const auto& rhs = *node.rhs_;
			for (size_t i = 0; i < rhs.size(); ++i) {
				print_expr(rhs[i]);
				if (i < rhs.size() - 1) {
					if constexpr (mode != AstPrintMode::Compress) {
                        append(", ");
					}else{
                        append(',');
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
		if constexpr (mode == AstPrintMode::Auto) {
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
	 * @note this function should be called only when line_start_ is true, as every indent is at the
	 * line start
	 *
	 */
	void indent() noexcept
	{
		static constexpr int  MAX_INDENT = 32;
		static constexpr char tabs[MAX_INDENT] =
			"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		append(tabs, indent_);
	}
	void space() noexcept { append(' '); }
	/**
	 * @brief Break line, and set line_start_ to true in non-compress mode
	 *
	 */
	void breakline() noexcept
	{
		if constexpr (mode == AstPrintMode::Compress) {
			append('\n');
		}
		else {
			if (comment_index_ < comment_tokens_->size()) {
				auto comment = comment_token();
				if (line_ == comment->line_) {
					// EmptyLine Shoundn't appear here, so no need to check
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