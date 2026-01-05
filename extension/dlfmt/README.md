---
title: dlfmt
description: A lua formatter with extremply high performance
pubDate: 2025-12-21
updatedDate: 2025-12-21
tags: ["lua","formatter","lua formatter","格式化","格式化工具","vscode 插件","高性能工具", "vscode extension", "high performance", "cpp"]
---

# dlfmt

> **dlfmt** is an extremely fast Lua code formatter written in C++,
> up to **50–500× faster than stylua** on large Lua files.

dlfmt focuses on **raw formatting performance** and **batch processing**.
It is designed for large-scale Lua codebases with **hundreds of thousands of lines**.

- 🚀 Written in **C++**
- ⚡ Optimized for **very large files**
- 📂 Format or compress **entire directories**
- 🧩 Supports **JSON-based task pipelines**
- 🛠 VS Code extension available

## Brief

## Performance

### Brief

| filename          | lines |  chars  | format-tool | Avg Time (s) |
|-------------------|-------|---------|-------------|--------------|
| hero_scripts.lua  | 29118 | 765150  | lua-format  | 0.9240       |
| hero_scripts.lua  | 29118 | 765150  | stylua      | 1.327        |
| hero_scripts.lua  | 29118 | 765150  | dlfmt       | 0.0184       |
| go_towers.lua     | 53804 | 1365652 | lua-format  | 16.771       |
| go_towers.lua     | 53804 | 1365652 | stylua      | 2.054        |
| go_towers.lua     | 53804 | 1365652 | dlfmt       | 0.0308       |
| lldebugger.lua    | 2548  | 60634   | lua-format  | 0.1044       |
| lldebugger.lua    | 2548  | 60634   | stylua      | 0.2059       |
| lldebugger.lua    | 2548  | 60634   | dlfmt       | 0.0030       |

### Detail

`hero_scripts.lua`: 29118 lines, 765150 chars.

```sh
hyperfine '/home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./kr1/hero_scripts.lua -i' --warmup 1
Benchmark 1: /home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./kr1/hero_scripts.lua -i
  Time (mean ± σ):     924.0 ms ±   7.5 ms    [User: 848.5 ms, System: 75.4 ms]
  Range (min … max):   913.7 ms … 934.8 ms    10 runs

hyperfine 'stylua ./kr1/hero_scripts.lua --syntax Lua52' --warmup 1
Benchmark 1: stylua ./kr1/hero_scripts.lua --syntax Lua52
  Time (mean ± σ):      1.327 s ±  0.019 s    [User: 0.968 s, System: 0.360 s]
  Range (min … max):    1.296 s …  1.351 s    10 runs

hyperfine 'dlfmt --format-file ./kr1/hero_scripts.lua' --warmup 1
Benchmark 1: dlfmt --format-file ./kr1/hero_scripts.lua
  Time (mean ± σ):      18.4 ms ±   0.8 ms    [User: 7.7 ms, System: 10.8 ms]
  Range (min … max):    17.0 ms …  21.6 ms    141 runs
```

`go_towers.lua`: 53804 lines, 1365652 chars.

```sh
hyperfine '/home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./_assets/kr1-desktop/images/fullhd/go_towers.lua -i' --warmup 1
Benchmark 1: /home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./_assets/kr1-desktop/images/fullhd/go_towers.lua -i
  Time (mean ± σ):     16.771 s ±  0.167 s    [User: 16.626 s, System: 0.145 s]
  Range (min … max):   16.546 s … 17.009 s    10 runs

hyperfine 'stylua ./_assets/kr1-desktop/images/fullhd/go_towers.lua --syntax Lua52' --warmup 1
Benchmark 1: stylua ./_assets/kr1-desktop/images/fullhd/go_towers.lua --syntax Lua52
  Time (mean ± σ):      2.054 s ±  0.031 s    [User: 0.701 s, System: 1.355 s]
  Range (min … max):    1.997 s …  2.083 s    10 runs

hyperfine 'dlfmt --format-file ./_assets/kr1-desktop/images/fullhd/go_towers.lua' --warmup 1
Benchmark 1: dlfmt --format-file ./_assets/kr1-desktop/images/fullhd/go_towers.lua
  Time (mean ± σ):      30.8 ms ±   1.0 ms    [User: 10.3 ms, System: 20.1 ms]
  Range (min … max):    29.2 ms …  34.7 ms    90 runs
```

`lldebugger.lua`: 2548 lines, 60634 chars.

```sh
hyperfine '/home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./lldebugger.lua -i' --warmup 1 --shell=none
Benchmark 1: /home/dove/.vscode-server/extensions/yinfei.luahelper-0.2.29/server/linux/lua-format ./lldebugger.lua -i
  Time (mean ± σ):     104.4 ms ±   1.4 ms    [User: 94.6 ms, System: 9.1 ms]
  Range (min … max):   101.8 ms … 107.2 ms    28 runs

hyperfine 'stylua ./lldebugger.lua --syntax Lua52' --warmup 1 --shell=none
Benchmark 1: stylua ./lldebugger.lua --syntax Lua52
  Time (mean ± σ):     205.9 ms ±   4.3 ms    [User: 121.3 ms, System: 86.7 ms]
  Range (min … max):   201.0 ms … 213.2 ms    14 runs

hyperfine 'dlfmt --format-file ./lldebugger.lua' --warmup 1 --shell=none
Benchmark 1: dlfmt --format-file ./lldebugger.lua
  Time (mean ± σ):       3.0 ms ±   0.3 ms    [User: 0.7 ms, System: 0.9 ms]
  Range (min … max):     2.5 ms …   6.2 ms    962 runs
```

## Usage

### Format a Single File: --format-file \<file\>

```sh
dlfmt --format-file ./tmp/hero_scripts.lua
[info dlfmt.cpp:442] Formatted file './tmp/hero_scripts.lua' in 38 ms.
```

### Format an Entire Directory: --format-directory \<directory\>

```sh
dlfmt --format-directory ./tmp/src-dlua
[info dlfmt.cpp:84] 1206 .lua files collected.
[info dlfmt.cpp:432] Formatted directory './tmp/src-dlua' in 357 ms.
```

### Compress a Single File: --compress-file \<file\>

```sh
dlfmt --compress-file ./tmp/hero_scripts.lua
[info dlfmt.cpp:462] Compressed file './tmp/hero_scripts.lua' in 36 ms.
```

### Compress an Entire Directory: --compress-directory \<directory\>

```sh
dlfmt --compress-directory ./tmp/src-dlua
[info dlfmt.cpp:150] 1206 .lua files collected.
[info dlfmt.cpp:452] Compressed directory './tmp/src-dlua' in 357 ms.
```

### Execute Formatting tasks: --json-task \<json_path\>

```sh
dlfmt --json-task ./task.json
[info dlfmt.cpp:316] 842 files to format collected.
[info dlfmt.cpp:317] 364 files to compress collected.
[info dlfmt.cpp:491] Processed json task './task.json' in 444 ms.
# Cache file .dlfmt_cache.json will be left in the same dir. So next time you launch json-task, you see:
dlfmt --json-task ./task.json
[info dlfmt.cpp:297] 0 files to format collected.
[info dlfmt.cpp:298] 0 files to compress collected.
[info dlfmt.cpp:472] Processed json task './task.json' in 15 ms.
```

The template for defining a formatting task is as follows:

```json
{
  "tasks": [
    {
      "type": "format",
      "directory": ".",
      "exclude": [
        "_assets/kr1-desktop/images/fullhd",
        "kr1/data/animations",
        "kr1/data/exoskeletons",
        "kr1/data/waves",
        "kr1/data/levels"
      ]
    },
    {
      "type": "compress",
      "directory": "_assets/kr1-desktop/images/fullhd"
    },
    {
      "type": "compress",
      "directory": "kr1/data/animations"
    },
    {
      "type": "compress",
      "directory": "kr1/data/exoskeletons"
    },
    {
      "type": "compress",
      "directory": "kr1/data/waves"
    },
    {
      "type": "compress",
      "directory": "kr1/data/levels"
    }
  ],
  "params": {
    "format": "manual",
    "compress": "auto"
  }
}
```

- `type` format: Specify a directory, format all .lua files under the directory.
- `type` compress: Specify a directory, compress all .lua files under the directory.
- `exclude`: exclude all directories listed in a single task.
- `params.format`: param for format tasks.
- `params.compress`: param for compress tasks.

## Formatting Effect

### Auto

When dlfmt works under auto mode, it format your code with the rules as follows:

- Add blank lines before and after block-level statements.
- Add blank lines between different groups of line-level statements. The Group types are shown as follow.

```cpp
enum class FormatStatGroup {
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
```

- All comments are attached before the statement, and aligned accordingly.
- Add spaces before and after operators, etc.

Below is a code snippet from lua-minify after auto formatting:

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
					if otherVar.Type == "Global" then
						otherVar.UsedNameArray[i] = true
					elseif otherVar.Type == "Local" then
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
--[[
print("Total Variables: "..#allVariables)
print("Total Range: "..rootScope.BeginLocation.."-"..rootScope.EndLocation)
print("")
for _, var in pairs(allVariables) do
	io.write("`"..var.Name.."':\n\t#symbols: "..#var.RenameList..
		"\n\tassigned to: "..tostring(var.AssignedTo))
	if var.Type == 'Local' then
		io.write("\n\trange: "..var.BeginLocation.."-"..var.EndLocation)
		io.write("\n\tlocal type: "..var.Info.Type)
	end
	io.write("\n\n")
end
-- First we want to rename all of the variables to unique temoraries, so that we can
-- easily use the scope::GetVar function to check whether renames are valid.
local temporaryIndex = 0
for _, var in pairs(allVariables) do
	var:Rename('_TMP_'..temporaryIndex..'_')
	temporaryIndex = temporaryIndex + 1
end
-- For each variable, we need to build a list of names that collide with it

error()
]]
end
```

### Manual

There's always some case that people get annoyed by certain auto breakline method. So manual mode for dlfmt formatting just remove the rules of "adding blank lines between different groups of line-level statements". However, continuous breaklines with an amount bigger than 2 will be rejected, back to 2.

## Compression Effect

Now dlfmt compression only compresses indentation and does not perform extra operations such as renaming variables. For debugging convenience, some line breaks are retained. All comments will be removed.

Below is a code snippet from lua-minify after compression:

```lua
local function MinifyVariables_2(globalScope,rootScope)
local globalUsedNames={}
for kw,_ in pairs(Keywords) do
globalUsedNames[kw]=true
end
local allVariables={}
local allLocalVariables={}
do
for _,var in pairs(globalScope) do
if var.AssignedTo then
table.insert(allVariables,var)
else
globalUsedNames[var.Name]=true
end
end
local function addFrom(scope)
for _,var in pairs(scope.VariableList) do
table.insert(allVariables,var)
table.insert(allLocalVariables,var)
end
for _,childScope in pairs(scope.ChildScopeList) do
addFrom(childScope)
end
end
addFrom(rootScope)
end
for _,var in pairs(allVariables) do
var.UsedNameArray={}
end
table.sort(allVariables,function(a,b)
return #a.RenameList<#b.RenameList
end)
local nextValidNameIndex=0
local varNamesLazy={}
local function varIndexToValidVarName(i)
local name=varNamesLazy[i]
if not name then
repeat
name=indexToVarName(nextValidNameIndex)
nextValidNameIndex=nextValidNameIndex+1
until not globalUsedNames[name]
varNamesLazy[i]=name
end
return name
end
for _,var in pairs(allVariables) do
var.Renamed=true
local i=1
while var.UsedNameArray[i] do
i=i+1
end
var:Rename(varIndexToValidVarName(i))
if var.Scope then
for _,otherVar in pairs(allVariables) do
if not otherVar.Renamed then
if not otherVar.Scope or otherVar.Scope.Depth<var.Scope.Depth then
for _,refAt in pairs(otherVar.ReferenceLocationList) do
if refAt>=var.BeginLocation and refAt<=var.ScopeEndLocation then
otherVar.UsedNameArray[i]=true
break
end
end
elseif otherVar.Scope.Depth>var.Scope.Depth then
for _,refAt in pairs(var.ReferenceLocationList) do
if refAt>=otherVar.BeginLocation and refAt<=otherVar.ScopeEndLocation then
otherVar.UsedNameArray[i]=true
break
end
end
else
if var.BeginLocation<otherVar.EndLocation and var.EndLocation>otherVar.BeginLocation then
otherVar.UsedNameArray[i]=true
end
end
end
end
else
for _,otherVar in pairs(allVariables) do
if not otherVar.Renamed then
if otherVar.Type=="Global" then
otherVar.UsedNameArray[i]=true
elseif otherVar.Type=="Local" then
for _,refAt in pairs(var.ReferenceLocationList) do
if refAt>=otherVar.BeginLocation and refAt<=otherVar.ScopeEndLocation then
otherVar.UsedNameArray[i]=true
break
end
end
else
assert(false,"unreachable")
end
end
end
end
end
end
```

## Extension Support

Search for extension in vscode: `dlfmt`

- For daily usage, just set `dlfmt` as your default formatter.
- In your file explorer, you can right click an item and choose the action to applied.For example, when right clicking a .json file, the item `dlfmt: run json tasks` is shown.