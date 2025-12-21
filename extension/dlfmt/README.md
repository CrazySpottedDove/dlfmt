# dlfmt

dlfmt 是 dl 系列衍生的 lua 代码格式化与压缩工具，性能优秀。如果你的项目对格式化性能要求极高，且不愿意配置格式化风格，可以尝试使用 dlfmt。其使用方法如下：

```sh
Usage: dlfmt [options]
Options:
  --help                 Show this help message and exit
  --version              Show version information and exit
  --format-file <file>   Format the specified file
  --format-directory <dir> Format all files in the specified directory recursively
  --compress-file <file>   Compress the specified file
  --compress-directory <dir> Compress all files in the specified directory recursively
  --json-task <file>     Process tasks defined in the specified JSON file
```

## 性能

`dlfmt` 的性能在 lua 格式化插件中属于顶尖水平。

| 文件名            | 行数  | 字符数  | 工具        | 平均时间 (s) |
|-------------------|-------|---------|-------------|--------------|
| hero_scripts.lua  | 29118 | 765150  | lua-format  | 1.1080       |
| hero_scripts.lua  | 29118 | 765150  | stylua      | 1.5490       |
| hero_scripts.lua  | 29118 | 765150  | dlfmt       | 0.0434       |
| go_towers.lua     | 53804 | 1365652 | lua-format  | 21.2510      |
| go_towers.lua     | 53804 | 1365652 | stylua      | 2.5120       |
| go_towers.lua     | 53804 | 1365652 | dlfmt       | 0.1080       |
| lldebugger.lua    | 2548  | 60634   | lua-format  | 0.1348       |
| lldebugger.lua    | 2548  | 60634   | stylua      | 0.2504       |
| lldebugger.lua    | 2548  | 60634   | dlfmt       | 0.0045       |

## 格式化效果

以下是 luaminify 的代码片段经过 dlfmt 格式化后的示例：

```lua
local function MinifyVariables_2(globalScope, rootScope)
	-- Variable names and other names that are fixed, that we cannot use
	-- Either these are Lua keywords, or globals that are not assigned to,
	-- that is environmental globals that are assigned elsewhere beyond our
	-- control.
	local globalUsedNames = {}

	for kw, _ in pairs(Keywords) do
		globalUsedNames[kw] = true
	end

	-- Gather a list of all of the variables that we will rename
	local allVariables = {}
	local allLocalVariables = {}

	do
		-- Add applicable globals
		for _, var in pairs(globalScope) do
			if var.AssignedTo then
				-- We can try to rename this global since it was assigned to
				-- (and thus presumably initialized) in the script we are
				-- minifying.
				table.insert(allVariables, var)
			else
				-- We can't rename this global, mark it as an unusable name
				-- and don't add it to the nename list
				globalUsedNames[var.Name] = true
			end
		end

		-- Recursively add locals, we can rename all of those
		local function addFrom(scope)
			for _, var in pairs(scope.VariableList) do
				table.insert(allVariables, var)
				table.insert(allLocalVariables, var)
			end

			for _, childScope in pairs(scope.ChildScopeList) do
				addFrom(childScope)
			end
		end

		addFrom(rootScope)
	end

	-- Add used name arrays to variables
	for _, var in pairs(allVariables) do
		var.UsedNameArray = {}
	end

	-- Sort the least used variables first
	table.sort(allVariables, function(a, b)
		return #a.RenameList < #b.RenameList
	end)

	-- Lazy generator for valid names to rename to
	local nextValidNameIndex = 0
	local varNamesLazy = {}

	local function varIndexToValidVarName(i)
		local name = varNamesLazy[i]

		if not name then
			repeat
				name = indexToVarName(nextValidNameIndex)
				nextValidNameIndex = nextValidNameIndex + 1
			until not globalUsedNames[name]

			varNamesLazy[i] = name
		end

		return name
	end

	-- For each variable, go to rename it
	for _, var in pairs(allVariables) do
		-- Lazy... todo: Make theis pair a proper for-each-pair-like set of loops
		-- rather than using a renamed flag.
		var.Renamed = true

		-- Find the first unused name
		local i = 1

		while var.UsedNameArray[i] do
			i = i + 1
		end

		-- Rename the variable to that name
		var:Rename(varIndexToValidVarName(i))

		if var.Scope then
			-- Now we need to mark the name as unusable by any variables:
			--  1) At the same depth that overlap lifetime with this one
			--  2) At a deeper level, which have a reference to this variable in their lifetimes
			--  3) At a shallower level, which are referenced during this variable's lifetime
			for _, otherVar in pairs(allVariables) do
				if not otherVar.Renamed then
					if not otherVar.Scope or otherVar.Scope.Depth < var.Scope.Depth then
						-- Check Global variable (Which is always at a shallower level)
						--  or
						-- Check case 3
						-- The other var is at a shallower depth, is there a reference to it
						-- durring this variable's lifetime?
						for _, refAt in pairs(otherVar.ReferenceLocationList) do
							if refAt >= var.BeginLocation and refAt <= var.ScopeEndLocation then
								-- Collide
								otherVar.UsedNameArray[i] = true

								break
							end
						end
					elseif otherVar.Scope.Depth > var.Scope.Depth then
						-- Check Case 2
						-- The other var is at a greater depth, see if any of the references
						-- to this variable are in the other var's lifetime.
						for _, refAt in pairs(var.ReferenceLocationList) do
							if refAt >= otherVar.BeginLocation and refAt <= otherVar.ScopeEndLocation then
								-- Collide
								otherVar.UsedNameArray[i] = true

								break
							end
						end
					else --otherVar.Scope.Depth must be equal to var.Scope.Depth
						-- Check case 1
						-- The two locals are in the same scope
						-- Just check if the usage lifetimes overlap within that scope. That is, we
						-- can shadow a local variable within the same scope as long as the usages
						-- of the two locals do not overlap.
						if var.BeginLocation < otherVar.EndLocation and var.EndLocation > otherVar.BeginLocation then
							otherVar.UsedNameArray[i] = true
						end
					end
				end
			end
		else
			-- This is a global var, all other globals can't collide with it, and
			-- any local variable with a reference to this global in it's lifetime
			-- can't collide with it.
			for _, otherVar in pairs(allVariables) do
				if not otherVar.Renamed then
					if otherVar.Type == 'Global' then
						otherVar.UsedNameArray[i] = true
					elseif otherVar.Type == 'Local' then
						-- Other var is a local, see if there is a reference to this global within
						-- that local's lifetime.
						for _, refAt in pairs(var.ReferenceLocationList) do
							if refAt >= otherVar.BeginLocation and refAt <= otherVar.ScopeEndLocation then
								-- Collide
								otherVar.UsedNameArray[i] = true

								break
							end
						end
					else
						assert(false, "unreachable")
					end
				end
			end
		end
	end
-- --
-- print("Total Variables: "..#allVariables)
-- print("Total Range: "..rootScope.BeginLocation.."-"..rootScope.EndLocation)
-- print("")
-- for _, var in pairs(allVariables) do
-- 	io.write("`"..var.Name.."':\n\t#symbols: "..#var.RenameList..
-- 		"\n\tassigned to: "..tostring(var.AssignedTo))
-- 	if var.Type == 'Local' then
-- 		io.write("\n\trange: "..var.BeginLocation.."-"..var.EndLocation)
-- 		io.write("\n\tlocal type: "..var.Info.Type)
-- 	end
-- 	io.write("\n\n")
-- end
-- -- First we want to rename all of the variables to unique temoraries, so that we can
-- -- easily use the scope::GetVar function to check whether renames are valid.
-- local temporaryIndex = 0
-- for _, var in pairs(allVariables) do
-- 	var:Rename('_TMP_'..temporaryIndex..'_')
-- 	temporaryIndex = temporaryIndex + 1
-- end
-- For each variable, we need to build a list of names that collide with it
--
--error()
end
```

压缩不处理变量名压缩。

## Json Task

可以在 json 文件中定义整个项目的格式化任务，以让 dlfmt 高效进行格式化。

示例的 `task.json` 文件：

```json
{
	"tasks": [
		{
			"type": "format",
			"directory": "tmp/src-dlua",
			"exclude": ["tmp/src-dlua/_assets"]
		},
		{
			"type": "compress",
			"directory": "tmp/src-dlua/_assets",
            "exclude": ["tmp/src-dlua/_assets/kr1-desktop"]
		}
	]
}
```

## [More: Click Here!](https://crazyspotteddove.github.io/projects/dlfmt/)