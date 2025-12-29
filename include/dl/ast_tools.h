#pragma once
#include "dl/ast.h"
#include "dl/token.h"
#include <cstdint>
#include <ostream>

namespace dl {
namespace AstTools {
class AstPrinter
{
public:
    enum class PrintMode : std::uint8_t
    {
        COMPRESS,
        AUTO,
        MANUAL
    };
    AstPrinter(std::ostream& out)
        : out_(out)
        , indent_(0)
    {}

    void PrintAst(const AstNode* ast);
    void PrintAst(const AstNode* ast, const std::vector<CommentToken>& comment_tokens);

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
    void print_token(const Token* token);
    void print_token_format_mode(const Token* token);
    void print_expr(const AstNode* expr);
    void print_expr_format_mode(const AstNode* expr);
    void print_stat(const AstNode* stat);
    void print_stat_format_mode(const AstNode* stat);
    void                set_format_stat_group(FormatStatGroup group) noexcept;
    void                do_format_stat_group_rules(const AstNode* stat) noexcept;
    FormatStatGroup     get_format_stat_group(const AstNode* stat) const noexcept;
    const CommentToken* comment_token();
    inline void         indent()
    {
        for (int i = 0; i < indent_; ++i) {
            out_.put('\t');
        }
    }
    inline void space() { out_.put(' '); }
    inline void breakline() { out_.put('\n'); }

    void        breakline_format_mode();
    bool        is_block_stat(AstNodeType type);
    inline void inc_indent()
    {
        ++indent_;
        is_block_start_ = true;
    }
    inline void dec_indent()
    {
        --indent_;
        is_block_start_ = false;
    }
    inline void enter_group_format_mode()
    {
        breakline_format_mode();
        ++indent_;
        set_format_stat_group(FormatStatGroup::None);
    }

    inline void exit_group_format_mode() { --indent_; }

    std::ostream&                    out_;
    int                              indent_;
    std::size_t                      line_                   = 1;
    std::size_t                      comment_index_          = 0;
    const std::vector<CommentToken>* comment_tokens_         = nullptr;
    bool                             line_start_             = true;
    bool                             last_is_block_stat_     = false;
    bool                             is_block_start_         = true;
    FormatStatGroup                  last_format_stat_group_ = FormatStatGroup::None;
};

void PrintAst(const AstNode* ast, std::ostream& out);
void PrintAst(const AstNode* ast, const std::vector<CommentToken>& comment_tokens,
              std::ostream& out);
}   // namespace AstTools
}   // namespace dl