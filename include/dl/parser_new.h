#pragma once
#pragma once

#include "dl/ast_manager.h"
#include "dl/ast_new.h"
#include "dl/token.h"
#include <cstddef>
#include <memory>
#include <vector>
namespace dl {
#define UNARY_PRIORITY 8
class Parser
{
public:
	Parser(std::vector<Token>& tokens, const std::string& file_name);
	AstNode* GetAstRoot() noexcept { return ast_root_; }

private:
	// 获得当前位置的 token，并将位置后移一位
	Token*      get() noexcept;
	Token*      peek(size_t offset = 0) const noexcept;
	std::string get_token_start_position(const Token* token) const noexcept;
	bool        is_block_follow() const noexcept;

	/**
	 * @brief 判断当前 token 是否为二元运算符
	 *
	 * @return true
	 * @return false
	 */
	bool is_binop() const noexcept;

	/**
	 * @brief 期待当前位置的 token 类型为 type，若是则消费，否则报错
	 *
	 * @param type
	 * @return Token*
	 */
	Token* expect(TokenType type);

	/**
	 * @brief 期待当前位置的 token 类型为 type 且值为 value，若是则消费，否则报错
	 *
	 * @param type
	 * @param value
	 * @return Token*
	 */
	Token* expect(TokenType type, const std::string_view value);

	/**
	 * @brief 报错并终止解析
	 *
	 * @param message
	 */
	[[noreturn]] void error(const std::string_view message);

	/**
	 * @brief 解析表达式列表
	 *
	 * @details a+b, c, d+e 解析为 expr_list=[a+b, c, d+e], comma_list=[',', ',']
	 * @param expr_list
	 * @param comma_list
	 */
	void exprlist(std::vector<AstNode*>& expr_list,
				  std::vector<Token*>&                   comma_list);
	/**
	 * @brief 解析前缀表达式
	 * @details (expr) 或 identifier
	 *
	 * @return AstNode*
	 */
	AstNode* prefixexpr();

	/**
	 * @brief 解析表构造表达式
	 * @details { ["a"] = 1, b = 2, 3, 4 }
	 *
	 * @return AstNode*
	 */
	AstNode* tableexpr();

	/**
	 * @brief 解析变量列表
	 * @details a, b, c 解析为 var_list=[a, b, c], comma_list=[',', ',']
	 *
	 * @param var_list
	 * @param comma_list
	 */
	void varlist(std::vector<Token*>& var_list, std::vector<Token*>& comma_list);

	/**
	 * @brief 解析代码块主体
	 *
	 * @param terminator
	 * @param body
	 * @param after
	 */
	void blockbody(std::string_view terminator, AstNode*& body, Token*& after);

	/**
	 * @brief 解析匿名函数声明
	 *
	 * @return AstNode*
	 */
	AstNode* funcdecl_anonymous();

	/**
	 * @brief 解析具名函数声明
	 *
	 * @return AstNode*
	 */
	AstNode* funcdecl_named();

	/**
	 * @brief 解析函数参数列表
	 *
	 * @return AstNode*
	 */
	AstNode* functionargs();

	/**
	 * @brief 解析主表达式
	 * @details a.* , a:*, a[*], a(*), a{*}
	 *
	 * @return AstNode*
	 */
	AstNode* primaryexpr();

	/**
	 * @brief 解析简单表达式
	 * @details 各种字面量，还有基本表达式
	 *
	 * @return AstNode*
	 */
	AstNode* simpleexpr();

	/**
	 * @brief 解析子表达式( a + b * c ^ d )，递归实现
	 *
	 * @param priority_limit
	 * @return AstNode*
	 */
	AstNode* subexpr(const size_t priority_limit);

	/**
	 * @brief 解析表达式的入口函数，可解析任何表达式
	 *
	 * @return AstNode*
	 */
	inline AstNode* expr();

	/**
	 * @brief 解析表达式语句
	 *
	 * @return AstNode*
	 */
	AstNode* exprstat();

	/**
	 * @brief 解析 if 语句
	 *
	 * @return AstNode*
	 */
	AstNode* ifstat();

	/**
	 * @brief 解析 do 语句
	 *
	 * @return AstNode*
	 */
	AstNode* dostat();

	/**
	 * @brief 解析 while 语句
	 *
	 * @return AstNode*
	 */
	AstNode* whilestat();

	/**
	 * @brief 解析 for 语句
	 *
	 * @return AstNode*
	 */
	AstNode* forstat();

	/**
	 * @brief 解析 repeat 语句
	 *
	 * @return AstNode*
	 */
	AstNode* repeatstat();

	/**
	 * @brief 解析局部变量声明语句
	 *
	 * @return AstNode*
	 */
	AstNode* localdecl();

	/**
	 * @brief 解析返回语句
	 *
	 * @return AstNode*
	 */
	AstNode* retstat();

	/**
	 * @brief 解析 break 语句
	 *
	 * @return AstNode*
	 */
	AstNode* breakstat();

	/**
	 * @brief 解析 goto 语句
	 *
	 * @return AstNode*
	 */
	AstNode* gotostat();

	/**
	 * @brief 解析 label 语句
	 *
	 */
	AstNode* labelstat();

	/**
	 * @brief 解析单条语句
	 *
	 * @param is_last
	 * @return AstNode*
	 */
	AstNode* statement(bool& is_last);

	/**
	 * @brief 解析代码块
	 *
	 * @return AstNode*
	 */
	AstNode* block();
	std::string              file_name_;
	size_t                   position_;
	std::vector<Token>&      tokens_;
	AstNode*                 ast_root_;
	AstManager               ast_manager_;
	bool                     reached_eof_;
};
}   // namespace dl