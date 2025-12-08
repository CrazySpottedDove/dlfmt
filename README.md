# DL

dl 的目标是制作一个可以生成运行性能更好的 lua 代码的 lua 编译器。当然，它也会有一些副产品。

## dlfmt

dlfmt 是 dl 系列衍生的 lua 代码格式化与压缩工具，性能优秀。dlfmt 暂时不开放可配置项，且会武断地处理空行，如果你的项目对格式化性能要求极高，且对于格式化风格无所谓，可以尝试使用 dlfmt。其使用方法如下：

```sh
Usage: dlfmt [options]
Options:
  --help                 Show this help message and exit
  --version              Show version information and exit
  --format-file <file>   Format the specified file
  --format-directory <dir> Format all files in the specified directory recursively
  --compress-file <file>   Compress the specified file
  --compress-directory <dir> Compress all files in the specified directory recursively
```

以下是 luaminify 的代码片段经过 dlfmt 格式化后的示例：

```lua
local function usageError()
	error(
		"\nusage: minify <file> or unminify <file>\n"
			.. "  The modified code will be printed to the stdout, pipe it to a file, the\n"
			.. "  lua interpreter, or something else as desired EG:\n\n"
			.. "        lua minify.lua minify input.lua > output.lua\n\n"
			.. "  * minify will minify the code in the file.\n"
			.. "  * unminify will beautify the code and replace the variable names with easily\n"
			.. "    find-replacable ones to aide in reverse engineering minified code.\n",
		0
	)
end

local args = { ... }

if #args ~= 2 then
	usageError()
end

local sourceFile = io.open(args[2], "r")

if not sourceFile then
	error("Could not open the input file `" .. args[2] .. "`", 0)
end

local data = sourceFile:read("*all")
local ast = CreateLuaParser(data)
local global_scope, root_scope = AddVariableInfo(ast)

local function minify(ast, global_scope, root_scope)
	MinifyVariables(global_scope, root_scope)
	StripAst(ast)
	PrintAst(ast)
end

local function beautify(ast, global_scope, root_scope)
	BeautifyVariables(global_scope, root_scope)
	FormatAst(ast)
	PrintAst(ast)
end

if args[1] == "minify" then
	minify(ast, global_scope, root_scope)
elseif args[1] == "unminify" then
	beautify(ast, global_scope, root_scope)
else
	usageError()
end
```

压缩不处理变量名压缩。