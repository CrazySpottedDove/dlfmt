#pragma once
#include "dl/ast.h"
#include "dl/token.h"
#include <cstdint>
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

    void PrintAst(const AstNode* ast){
        print_stat(ast);
        if constexpr (mode != AstPrintMode::Compress) {
            // 注意把文件末尾的注释也打印出来
            while (comment_index_ < comment_tokens_->size()) {
                out_.put('\n');
                indent();
                out_ << comment_token()->source_;
                ++comment_index_;
            }
        }
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
    void print_token(const Token* token)
    {
        if constexpr (mode == AstPrintMode::Compress) {
            out_ << token->source_;
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
                        out_ << comment->source_;
                        out_.put('\n');
                        ++comment_index_;
                        indent();
                    }
                    else {
                        break;
                    }
                }
                line_start_ = false;
            }
            out_ << token->source_;
        }
    }
    // void                print_token_format_mode(const Token* token);
    void print_expr(const AstNode* expr)
    {
        const auto type = expr->GetType();
        if (type == AstNodeType::BinopExpr) {
            auto node = static_cast<const BinopExpr*>(expr);
            if constexpr (mode == AstPrintMode::Compress) {
                const bool need_space =
                    (node->token_op_->source_ == "and" || node->token_op_->source_ == "or");
                print_expr(node->lhs_.get());
                if (need_space) {
                    space();
                    print_token(node->token_op_);
                    space();
                }
                else {
                    print_token(node->token_op_);
                }
            }
            else if constexpr (mode == AstPrintMode::Auto) {
                print_expr(node->lhs_.get());
                space();
                print_token(node->token_op_);
                space();
            }

            print_expr(node->rhs_.get());
            return;
        }
        if (type == AstNodeType::UnopExpr) {
            auto node = static_cast<const UnopExpr*>(expr);
            print_token(node->token_op_);
            if (node->token_op_->source_ == "not") {
                space();
            }
            print_expr(node->rhs_.get());
            return;
        }
        if (type == AstNodeType::NumberLiteral || type == AstNodeType::StringLiteral ||
            type == AstNodeType::NilLiteral || type == AstNodeType::BooleanLiteral ||
            type == AstNodeType::VargLiteral) {
            print_token(expr->GetFirstToken());
            return;
        }
        if (type == AstNodeType::FieldExpr) {
            auto node = static_cast<const FieldExpr*>(expr);
            print_expr(node->base_.get());
            print_token(node->token_dot_);
            print_token(node->field_);
            return;
        }
        if (type == AstNodeType::IndexExpr) {
            auto node = static_cast<const IndexExpr*>(expr);
            print_expr(node->base_.get());
            print_token(node->token_open_bracket_);
            print_expr(node->index_.get());
            print_token(node->token_close_bracket_);
            return;
        }
        if (type == AstNodeType::MethodExpr) {
            auto node = static_cast<const MethodExpr*>(expr);
            print_expr(node->base_.get());
            print_token(node->token_colon_);
            print_token(node->method_);

            const auto function_args = node->function_arguments_.get();
            const auto call_type     = function_args->GetType();
            if (call_type == AstNodeType::StringCall) {
                print_token(static_cast<const StringCall*>(function_args)->token_);
            }
            else if (call_type == AstNodeType::ArgCall) {
                auto arg_call = static_cast<const ArgCall*>(function_args);
                print_token(arg_call->token_open_paren_);
                for (size_t i = 0; i < arg_call->arg_list_.size(); ++i) {
                    print_expr(arg_call->arg_list_[i].get());
                    if (i < arg_call->token_comma_list_.size()) {
                        print_token(arg_call->token_comma_list_[i]);
                        if constexpr (mode != AstPrintMode::Compress) {
                            space();
                        }
                    }
                }
                print_token(arg_call->token_close_paren_);
            }
            else if (call_type == AstNodeType::TableCall) {
                print_expr(static_cast<const TableCall*>(function_args)->table_expr_.get());
            }
            return;
        }
        if (type == AstNodeType::CallExpr) {
            auto node = static_cast<const CallExpr*>(expr);
            print_expr(node->base_.get());

            const auto function_args = node->function_arguments_.get();
            const auto call_type     = function_args->GetType();
            if (call_type == AstNodeType::StringCall) {
                print_token(static_cast<const StringCall*>(function_args)->token_);
            }
            else if (call_type == AstNodeType::ArgCall) {
                auto arg_call = static_cast<const ArgCall*>(function_args);
                print_token(arg_call->token_open_paren_);
                for (size_t i = 0; i < arg_call->arg_list_.size(); ++i) {
                    print_expr(arg_call->arg_list_[i].get());
                    if (i < arg_call->token_comma_list_.size()) {
                        print_token(arg_call->token_comma_list_[i]);
                        if constexpr (mode != AstPrintMode::Compress) {
                            space();
                        }
                    }
                }
                print_token(arg_call->token_close_paren_);
            }
            else if (call_type == AstNodeType::TableCall) {
                print_expr(static_cast<const TableCall*>(function_args)->table_expr_.get());
            }
            return;
        }
        if (type == AstNodeType::FunctionLiteral) {
            auto node = static_cast<const FunctionLiteral*>(expr);
            print_token(node->token_function_);
            print_token(node->token_open_paren_);
            for (size_t i = 0; i < node->arg_list_.size(); ++i) {
                print_token(node->arg_list_[i]);
                if (i < node->token_arg_comma_list_.size()) {
                    print_token(node->token_arg_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            print_token(node->token_close_paren_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);
            return;
        }
        if (type == AstNodeType::VariableExpr) {
            print_token(static_cast<const VariableExpr*>(expr)->token_);
            return;
        }
        if (type == AstNodeType::ParenExpr) {
            auto node = static_cast<const ParenExpr*>(expr);
            print_token(node->token_open_paren_);
            print_expr(node->expression_.get());
            print_token(node->token_close_paren_);
            return;
        }
        if (type == AstNodeType::TableLiteral) {
            auto node = static_cast<const TableLiteral*>(expr);
            print_token(node->token_open_brace_);
            if (!node->entry_list_.empty()) {
                if constexpr (mode == AstPrintMode::Compress) {
                    for (size_t i = 0; i < node->entry_list_.size(); ++i) {
                        auto       entry      = node->entry_list_[i].get();
                        const auto entry_type = entry->GetType();
                        if (entry_type == TableEntryType::Field) {
                            auto field_entry = static_cast<const FieldEntry*>(entry);
                            print_token(field_entry->field_);

                            print_token(field_entry->token_equals_);

                            print_expr(field_entry->value_.get());
                        }
                        else if (entry_type == TableEntryType::Index) {
                            auto index_entry = static_cast<const IndexEntry*>(entry);
                            print_token(index_entry->token_open_bracket_);
                            print_expr(index_entry->index_.get());
                            print_token(index_entry->token_close_bracket_);

                            print_token(index_entry->token_equals_);

                            print_expr(index_entry->value_.get());
                        }
                        else if (entry_type == TableEntryType::Value) {
                            auto value_entry = static_cast<const ValueEntry*>(entry);
                            print_expr(value_entry->value_.get());
                        }
                        // Other entry type UNREACHABLE
                        if (i < node->token_separator_list_.size()) {
                            print_token(node->token_separator_list_[i]);
                        }
                    }
                }
                else if constexpr (mode == AstPrintMode::Auto) {
                    // 对于纯 value entry 且较短的表，尝试一行输出
                    bool one_line = true;
                    if (node->entry_list_.size() > 10) {
                        one_line = false;
                    }
                    else {
                        for (size_t i = 0; i < node->entry_list_.size(); ++i) {
                            const auto entry_type = node->entry_list_[i]->GetType();
                            if (entry_type != TableEntryType::Value) {
                                one_line = false;
                                break;
                            }
                        }
                    }

                    if (one_line) {
                        // 单行输出
                        for (size_t i = 0; i < node->entry_list_.size(); ++i) {
                            auto entry       = node->entry_list_[i].get();
                            auto value_entry = static_cast<const ValueEntry*>(entry);
                            print_expr(value_entry->value_.get());
                            // Other entry type UNREACHABLE
                            if (i < node->entry_list_.size() - 1) {
                                print_token(node->token_separator_list_[i]);
                                space();
                            }
                        }
                    }
                    else {
                        breakline();
                        inc_indent();
                        for (size_t i = 0; i < node->entry_list_.size(); ++i) {
                            auto       entry      = node->entry_list_[i].get();
                            const auto entry_type = entry->GetType();
                            indent();
                            if (entry_type == TableEntryType::Field) {
                                auto field_entry = static_cast<const FieldEntry*>(entry);
                                print_token(field_entry->field_);
                                space();
                                print_token(field_entry->token_equals_);
                                space();
                                print_expr(field_entry->value_.get());
                            }
                            else if (entry_type == TableEntryType::Index) {
                                auto index_entry = static_cast<const IndexEntry*>(entry);
                                print_token(index_entry->token_open_bracket_);
                                print_expr(index_entry->index_.get());
                                print_token(index_entry->token_close_bracket_);
                                space();
                                print_token(index_entry->token_equals_);
                                space();
                                print_expr(index_entry->value_.get());
                            }
                            else if (entry_type == TableEntryType::Value) {
                                auto value_entry = static_cast<const ValueEntry*>(entry);
                                print_expr(value_entry->value_.get());
                            }
                            // Other entry type UNREACHABLE
                            if (i < node->entry_list_.size() - 1) {
                                print_token(node->token_separator_list_[i]);
                            }
                            breakline();
                        }
                        dec_indent();
                        indent();
                    }
                }
            }
            print_token(node->token_close_brace_);
            return;
        }
    }
    // void                print_expr_format_mode(const AstNode* expr);
    void print_stat(const AstNode* stat)
    {
        if (stat->GetType() == AstNodeType::StatList) {
            auto node = static_cast<const StatList*>(stat);
            for (auto& stat : node->statement_list) {
                print_stat(stat.get());
            }
            return;
        }

        if constexpr (mode != AstPrintMode::Compress) {
            do_format_stat_group_rules(stat);
            indent();
        }

        if (stat->GetType() == AstNodeType::BreakStat) {
            print_token(stat->GetFirstToken());
        }
        else if (stat->GetType() == AstNodeType::ReturnStat) {
            auto node = static_cast<const ReturnStat*>(stat);
            print_token(node->token_return_);
            if (!node->expr_list_.empty()) {
                space();
                for (size_t i = 0; i < node->expr_list_.size(); ++i) {
                    print_expr(node->expr_list_[i].get());
                    if (i < node->token_comma_list_.size()) {
                        print_token(node->token_comma_list_[i]);
                        space();
                    }
                }
            }
        }
        else if (stat->GetType() == AstNodeType::LocalVarStat) {
            auto node = static_cast<const LocalVarStat*>(stat);
            print_token(node->token_local_);
            space();
            for (size_t i = 0; i < node->var_list_.size(); ++i) {
                print_token(node->var_list_[i]);
                if (i < node->token_var_comma_list_.size()) {
                    print_token(node->token_var_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            if (node->token_equals_) {
                if constexpr (mode != AstPrintMode::Compress) {
                    space();
                    print_token(node->token_equals_);
                    space();
                }
                else {
                    print_token(node->token_equals_);
                }
                for (size_t i = 0; i < node->expr_list_.size(); ++i) {
                    print_expr(node->expr_list_[i].get());
                    if (i < node->token_expr_comma_list_.size()) {
                        print_token(node->token_expr_comma_list_[i]);
                        if constexpr (mode != AstPrintMode::Compress) {
                            space();
                        }
                    }
                }
            }
        }
        else if (stat->GetType() == AstNodeType::LocalFunctionStat) {
            auto node = static_cast<const LocalFunctionStat*>(stat);
            print_token(node->token_local_);
            space();
            auto function_node = static_cast<const FunctionStat*>(node->function_stat_.get());
            print_token(function_node->token_function_);
            space();
            print_token(function_node->name_chain_[0]);
            print_token(function_node->token_open_paren_);
            for (size_t i = 0; i < function_node->arg_list_.size(); ++i) {
                print_token(function_node->arg_list_[i]);
                if (i < function_node->token_arg_comma_list_.size()) {
                    print_token(function_node->token_arg_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            print_token(function_node->token_close_paren_);
            enter_group();
            print_stat(function_node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(function_node->token_end_);
        }
        else if (stat->GetType() == AstNodeType::FunctionStat) {
            auto node = static_cast<const FunctionStat*>(stat);
            print_token(node->token_function_);
            space();
            for (size_t i = 0; i < node->name_chain_.size(); ++i) {
                print_token(node->name_chain_[i]);
                if (i < node->token_name_chain_separator_.size()) {
                    print_token(node->token_name_chain_separator_[i]);
                }
            }
            print_token(node->token_open_paren_);
            for (size_t i = 0; i < node->arg_list_.size(); ++i) {
                print_token(node->arg_list_[i]);
                if (i < node->token_arg_comma_list_.size()) {
                    print_token(node->token_arg_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            print_token(node->token_close_paren_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);
        }
        else if (stat->GetType() == AstNodeType::RepeatStat) {
            auto node = static_cast<const RepeatStat*>(stat);
            print_token(node->token_repeat_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_until_);
            space();
            print_expr(node->condition_.get());
        }
        else if (stat->GetType() == AstNodeType::GenericForStat) {
            auto node = static_cast<const GenericForStat*>(stat);
            print_token(node->token_for_);
            space();
            for (size_t i = 0; i < node->var_list_.size(); ++i) {
                print_token(node->var_list_[i]);
                if (i < node->token_var_comma_list_.size()) {
                    print_token(node->token_var_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            space();
            print_token(node->token_in_);
            space();
            for (size_t i = 0; i < node->generator_list_.size(); ++i) {
                print_expr(node->generator_list_[i].get());
                if (i < node->token_generator_comma_list_.size()) {
                    print_token(node->token_generator_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            space();
            print_token(node->token_do_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);
        }
        else if (stat->GetType() == AstNodeType::NumericForStat) {
            auto node = static_cast<const NumericForStat*>(stat);
            print_token(node->token_for_);
            space();
            for (size_t i = 0; i < node->var_list_.size(); ++i) {
                print_token(node->var_list_[i]);
                if (i < node->token_var_comma_list_.size()) {
                    print_token(node->token_var_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            if constexpr (mode != AstPrintMode::Compress) {
                space();
                print_token(node->token_equals_);
                space();
            }
            else {
                print_token(node->token_equals_);
            }
            for (size_t i = 0; i < node->range_list_.size(); ++i) {
                print_expr(node->range_list_[i].get());
                if (i < node->token_range_comma_list_.size()) {
                    print_token(node->token_range_comma_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            space();
            print_token(node->token_do_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);
        }
        else if (stat->GetType() == AstNodeType::WhileStat) {
            auto node = static_cast<const WhileStat*>(stat);
            print_token(node->token_while_);
            space();
            print_expr(node->condition_.get());
            space();
            print_token(node->token_do_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);

        }
        else if (stat->GetType() == AstNodeType::DoStat) {
            auto node = static_cast<const DoStat*>(stat);
            print_token(node->token_do_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            print_token(node->token_end_);
        }else if (stat->GetType() == AstNodeType::IfStat) {
            auto node = static_cast<const IfStat*>(stat);
            print_token(node->token_if_);
            space();
            print_expr(node->condition_.get());
            space();
            print_token(node->token_then_);
            enter_group();
            print_stat(node->body_.get());
            exit_group();
            if constexpr (mode != AstPrintMode::Compress) {
                indent();
            }
            for (size_t i = 0; i < node->else_clause_list_.size(); ++i) {
                auto clause = node->else_clause_list_[i].get();
                print_token(clause->token_);
                if (clause->GetCondition()) {
                    space();
                    print_expr(clause->GetCondition());
                    space();
                    print_token(clause->GetTokenThen());
                }
                enter_group();
                print_stat(clause->body_.get());
                exit_group();
                if constexpr (mode != AstPrintMode::Compress) {
                    indent();
                }
            }
            print_token(node->token_end_);
        }
        else if (stat->GetType() == AstNodeType::CallExprStat) {
            auto node = static_cast<const CallExprStat*>(stat);
            print_expr(node->expression_.get());
        }
        else if (stat->GetType() == AstNodeType::AssignmentStat) {
            auto node = static_cast<const AssignmentStat*>(stat);
            for (size_t i = 0; i < node->lhs_.size(); ++i) {
                print_expr(node->lhs_[i].get());
                if (i < node->token_lhs_separator_list_.size()) {
                    print_token(node->token_lhs_separator_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
            if constexpr (mode != AstPrintMode::Compress) {
                space();
                print_token(node->token_equals_);
                space();
            }
            else {
                print_token(node->token_equals_);
            }
            for (size_t i = 0; i < node->rhs_.size(); ++i) {
                print_expr(node->rhs_[i].get());
                if (i < node->token_rhs_separator_list_.size()) {
                    print_token(node->token_rhs_separator_list_[i]);
                    if constexpr (mode != AstPrintMode::Compress) {
                        space();
                    }
                }
            }
        }
        else if (stat->GetType() == AstNodeType::GotoStat) {
            auto node = static_cast<const GotoStat*>(stat);
            print_token(node->token_goto_);
            space();
            print_token(node->token_label_);
        }
        else if (stat->GetType() == AstNodeType::LabelStat) {
            auto node = static_cast<const LabelStat*>(stat);
            print_token(node->token_label_start_);
            print_token(node->token_label_);
            print_token(node->token_label_end_);
        }
        breakline();
        if constexpr (mode != AstPrintMode::Compress) {
            set_format_stat_group(get_format_stat_group(stat));
        }
        return;
    }
    // void                print_stat_format_mode(const AstNode* stat);
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
                out_.put('\n');
                return;
            }

            // 如果组不同，则换行
            if (stat_group != last_format_stat_group_) {
                // 不同组，插入空行
                out_.put('\n');
            }
        }
    }
    FormatStatGroup get_format_stat_group(const AstNode* stat) const noexcept
    {
        switch (stat->GetType()) {
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
    const CommentToken* comment_token() { return &(*comment_tokens_)[comment_index_]; }
    void                indent()
    {
        for (int i = 0; i < indent_; ++i) {
            out_.put('\t');
        }
    }
    void space() { out_.put(' '); }
    void breakline()
    {
        if constexpr (mode == AstPrintMode::Compress) {
            out_.put('\n');
        }
        else {
            if (comment_index_ < comment_tokens_->size()) {
                auto comment = comment_token();
                if (line_ == comment->line_) {
                    // 发现当前行有注释，在行末追加
                    space();
                    out_ << comment->source_;
                    ++comment_index_;
                }
            }
            out_.put('\n');
            line_start_ = true;
        }
    }

    // void        breakline_format_mode();
    bool is_block_stat(AstNodeType type)
    {
        static const std::unordered_set<AstNodeType> block_stats_ = {
            AstNodeType::LocalFunctionStat,
            AstNodeType::FunctionStat,
            AstNodeType::RepeatStat,
            AstNodeType::GenericForStat,
            AstNodeType::NumericForStat,
            AstNodeType::WhileStat,
            AstNodeType::DoStat,
            AstNodeType::IfStat,
        };
        return block_stats_.find(type) != block_stats_.end();
    }
    void inc_indent()
    {
        ++indent_;
        is_block_start_ = true;
    }
    void dec_indent()
    {
        --indent_;
        is_block_start_ = false;
    }
    // inline void enter_group_format_mode()
    // {
    //     breakline_format_mode();
    //     ++indent_;
    //     set_format_stat_group(FormatStatGroup::None);
    // }
    void enter_group()
    {
        breakline();
        if constexpr (mode != AstPrintMode::Compress) {
            ++indent_;
            set_format_stat_group(FormatStatGroup::None);
        }
    }

    // inline void exit_group_format_mode() { --indent_; }
    void exit_group()
    {
        if constexpr (mode != AstPrintMode::Compress) {
            --indent_;
        }
    }

    std::ostream&                    out_;
    std::size_t                      line_                   = 1;
    std::size_t                      comment_index_          = 0;
    const std::vector<CommentToken>* comment_tokens_         = nullptr;
    int                              indent_;
    FormatStatGroup                  last_format_stat_group_ = FormatStatGroup::None;
    bool                             line_start_             = true;
    bool                             last_is_block_stat_     = false;
    bool                             is_block_start_         = true;
};
}   // namespace dl