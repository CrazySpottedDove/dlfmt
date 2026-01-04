#include "dlfmt_core.h"
#include "dl/ast_printer.h"
#include "dl/parser.h"
#include "dl/tokenizer.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <unordered_map>
#include <vector>
static constexpr const char* VERSION = "0.1.2";
using namespace dl;
void ShowHelp()
{
	printf("Usage: dlfmt [options]\n");
	printf("Options:\n");
	printf("  --help                 Show this help message and exit\n");
	printf("  --version              Show version information and exit\n");
	printf("  --format-file <file>   Format the specified file\n");
	printf("  --format-directory <dir> Format all files in the specified directory recursively\n");
	printf("  --compress-file <file>   Compress the specified file\n");
	printf(
		"  --compress-directory <dir> Compress all files in the specified directory recursively\n");
	printf("  --json-task <file>     Process tasks defined in the specified JSON file\n");
	printf("  --param <parameter>    Specify additional parameters for formatting/compressing\n");
}

void ShowVersion()
{
	printf("dlfmt version %s\n", VERSION);
}

void FormatFile(const std::string& format_file, dlfmt_param param)
{

	std::ifstream file(format_file, std::ios::binary);
	if (!file) {
		SPDLOG_ERROR("Failed to open file: {}", format_file.c_str());
		throw std::runtime_error("Failed to open file: " + format_file);
	}

	file.seekg(0, std::ios::end);
	const auto size = static_cast<size_t>(file.tellg());
	file.seekg(0);
	std::string content;
	if (size) {
		content.resize(size);
		file.read(&content[0], static_cast<std::streamsize>(size));
	}
	file.close();

	switch (param) {
	case dlfmt_param::manual_format:
	{
		// tokenize
		Tokenizer<TokenizeMode::FormatManual> tokenizer(std::move(content), format_file);

#ifndef NDEBUG
		tokenizer.Print();
#endif

		// parse
		Parser parser(tokenizer.getTokens(), format_file);

		// 打开输出
		std::ofstream out_file(format_file, std::ios::binary | std::ios::trunc);

		// 写入
		AstPrinter<AstPrintMode::Manual> printer(out_file, &tokenizer.getCommentTokens());
		printer.PrintAst(parser.GetAstRoot());
		out_file.flush();
		out_file.close();
		break;
	}
	default:
	{
		// tokenize
		Tokenizer<TokenizeMode::FormatAuto> tokenizer(std::move(content), format_file);

		// parse
		Parser parser(tokenizer.getTokens(), format_file);

		// 打开输出
		std::ofstream out_file(format_file, std::ios::binary | std::ios::trunc);

		// 写入
		AstPrinter<AstPrintMode::Auto> printer(out_file, &tokenizer.getCommentTokens());
		printer.PrintAst(parser.GetAstRoot());
		out_file.flush();
		out_file.close();
	}
	}
}

void FormatDirectory(const std::string& format_directory, dlfmt_param param)
{
	if (format_directory.empty()) {
		SPDLOG_ERROR("No directory specified for formatting.");
		throw std::invalid_argument("No directory specified for formatting.");
	}

	// 收集所有 .lua 文件
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(format_directory)) {
		if (entry.is_regular_file()) {
			const auto& path = entry.path();
			if (path.has_extension() && path.extension() == ".lua") {
				files.emplace_back(path.string());
			}
		}
	}
	SPDLOG_INFO("{} .lua files collected.", files.size());

// 并行格式化
#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(files.size()); ++i) {
		try {
			FormatFile(files[i], param);
		}
		catch (const std::exception& e) {
#pragma omp critical
			{
				SPDLOG_ERROR("Format failed: {} ({})", files[i], e.what());
			}
		}
		catch (...) {
#pragma omp critical
			{
				SPDLOG_ERROR("Format failed: {} (unknown error)", files[i]);
			}
		}
	}
}

void CompressFile(const std::string& compress_file, [[maybe_unused]] dlfmt_param param)
{
	std::ifstream file(compress_file, std::ios::binary);
	if (!file) {
		SPDLOG_ERROR("Failed to open file: {}", compress_file.c_str());
		throw std::runtime_error("Failed to open file: " + compress_file);
	}

	file.seekg(0, std::ios::end);
	const auto size = static_cast<size_t>(file.tellg());
	file.seekg(0);
	std::string content;
	if (size) {
		content.resize(size);
		file.read(&content[0], static_cast<std::streamsize>(size));
	}
	file.close();

	// tokenize
	Tokenizer<TokenizeMode::Compress> tokenizer(std::move(content), compress_file);

	// parse
	Parser parser(tokenizer.getTokens(), compress_file);

	// 打开输出
	std::ofstream out_file(compress_file, std::ios::binary | std::ios::trunc);

	// 写入
	AstPrinter<AstPrintMode::Compress> printer(out_file);
	printer.PrintAst(parser.GetAstRoot());
	out_file.flush();
	out_file.close();
}

void CompressDirectory(const std::string& compress_directory, [[maybe_unused]] dlfmt_param param)
{
	if (compress_directory.empty()) {
		SPDLOG_ERROR("No directory specified for formatting.");
		throw std::invalid_argument("No directory specified for formatting.");
	}

	// 收集所有 .lua 文件
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(compress_directory)) {
		if (entry.is_regular_file()) {
			const auto& path = entry.path();
			if (path.has_extension() && path.extension() == ".lua") {
				files.emplace_back(path.string());
			}
		}
	}
	SPDLOG_INFO("{} .lua files collected.", files.size());

// 并行格式化
#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(files.size()); ++i) {
		try {
			CompressFile(files[i], param);
		}
		catch (const std::exception& e) {
#pragma omp critical
			{
				SPDLOG_ERROR("Compress failed: {} ({})", files[i], e.what());
			}
		}
		catch (...) {
#pragma omp critical
			{
				SPDLOG_ERROR("Compress failed: {} (unknown error)", files[i]);
			}
		}
	}
}

using json         = nlohmann::json;
using file_cache_t = int64_t;

static int64_t FileTimeTypeToTimeT(std::filesystem::file_time_type t)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(
		t - std::filesystem::file_time_type::clock::now() + system_clock::now());
	return duration_cast<seconds>(sctp.time_since_epoch()).count();
}

// 只用mtime判断
static bool ShouldProcessFile(const std::string&                                   path,
							  const std::unordered_map<std::string, file_cache_t>& file_cache)
{
	// std::string     abspath = std::filesystem::absolute(path).string();
	std::error_code ec;
	// auto            mtime = std::filesystem::last_write_time(abspath, ec);
    const auto mtime = std::filesystem::last_write_time(path, ec);
	if (ec) return true;
	const int64_t s_mtime = FileTimeTypeToTimeT(mtime);
	// const auto    it      = file_cache.find(abspath);
    const auto    it      = file_cache.find(path);
	if (it != file_cache.end() && it->second == s_mtime) {
		return false;
	}
	return true;
}

void JsonTask(const std::string& json_file)
{
	// 加载任务缓存记录
	std::unordered_map<std::string, file_cache_t> file_cache;
	const std::string                             cache_path = ".dlfmt_cache.json";
	file_cache.clear();
	std::ifstream cache_in(cache_path);
	if (cache_in) {
		json cache_j;
		cache_in >> cache_j;
		for (auto& [k, v] : cache_j.items()) {
			file_cache[k] = {v.get<file_cache_t>()};
		}
		cache_in.close();
	}

	// 解析 dlua_task.json
	std::ifstream task_in(json_file);
	if (!task_in) throw std::runtime_error("Failed to open json task file");
	json task_j;
	task_in >> task_j;
	task_in.close();

	dlfmt_param param_format   = dlfmt_param::auto_format;
	dlfmt_param param_compress = dlfmt_param::auto_format;
	if (task_j.contains("params")) {
		auto params = task_j["params"];
		if (params.contains("format")) {
			std::string fmt_param = params["format"];
			if (fmt_param == "manual") {
				param_format = dlfmt_param::manual_format;
			}
		}

		if (params.contains("compress")) {
			std::string cmp_param = params["compress"];
			// TODO: no param available for compress now
		}
	}

	auto tasks = task_j["tasks"];

	std::vector<std::string> format_tasks;
	std::vector<std::string> compress_tasks;

	std::filesystem::path work_dir = std::filesystem::current_path();

	// 直接先收集任务
	for (const auto& task : tasks) {
		if (task["type"] == "compress") {
			std::vector<std::filesystem::path> exclude;
			if (task.contains("exclude")) {
				for (const auto& ex : task["exclude"])
					// exclude.push_back(std::filesystem::absolute(work_dir / ex.get<std::string>()));
                exclude.push_back(std::filesystem::path(ex.get<std::string>()));
			}
			// 遍历目录，收集所有lua文件
			for (const auto& entry :
				 std::filesystem::recursive_directory_iterator(task["directory"])) {
				if (entry.is_regular_file() && entry.path().extension() == ".lua") {
					// std::string abs_path =
						// std::filesystem::absolute(entry.path().string()).string();
                    std::string path = entry.path().string();
					// 文件没有变，不需要加入任务清单
					// if (!ShouldProcessFile(abs_path, file_cache)) {
						// continue;
					// }
                    if(!ShouldProcessFile(entry.path().string(), file_cache)) {
                        continue;
                    }

					bool is_excluded = false;
					for (const auto& ex : exclude) {
						// if (abs_path.compare(0, ex.string().size(), ex.string()) == 0) {
                        if(path.compare(0, ex.string().size(), ex.string()) == 0) {
							is_excluded = true;
							break;
						}
					}
					// 文件路径被排除，不加入任务清单
					if (is_excluded) {
						continue;
					}
					// compress_tasks.push_back(abs_path);
                    compress_tasks.push_back(path);
				}
			}
		}
		else if (task["type"] == "format") {
			std::vector<std::filesystem::path> exclude;
			if (task.contains("exclude")) {
				for (const auto& ex : task["exclude"]) {
					// exclude.push_back(std::filesystem::absolute(work_dir / ex.get<std::string>()));
                    exclude.push_back(std::filesystem::path(ex.get<std::string>()));
				}
			}

			// 遍历目录，收集所有lua文件
			for (const auto& entry :
				 std::filesystem::recursive_directory_iterator(task["directory"])) {
				if (entry.is_regular_file() && entry.path().extension() == ".lua") {
					// std::string abs_path =
						// std::filesystem::absolute(entry.path().string()).string();
                    std::string path = entry.path().string();

					// 文件没有变，不需要加入任务清单
					// if (!ShouldProcessFile(abs_path, file_cache)) {
                    if (!ShouldProcessFile(path, file_cache)) {
						continue;
					}

					bool is_excluded = false;

					for (const auto& ex : exclude) {
						// if (abs_path.compare(0, ex.string().size(), ex.string()) == 0) {
                        if (path.compare(0, ex.string().size(), ex.string()) == 0) {
							is_excluded = true;
							break;
						}
					}
					// 文件路径被排除，不加入任务清单
					if (is_excluded) {
						continue;
					}
					// format_tasks.push_back(abs_path);
                    format_tasks.push_back(path);
				}
			}
		}
	}

	SPDLOG_INFO("{} files to format collected.", format_tasks.size());
	SPDLOG_INFO("{} files to compress collected.", compress_tasks.size());

// 然后处理任务。先 format，后 compress
#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(format_tasks.size()); ++i) {
		const auto& abs_path = format_tasks[i];
		FormatFile(abs_path, param_format);
	}

#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(compress_tasks.size()); ++i) {
		const auto& abs_path = compress_tasks[i];
		CompressFile(abs_path, param_compress);
	}

	for (const auto& abs_path : format_tasks) {
		std::error_code ec;
		auto            mtime = std::filesystem::last_write_time(abs_path, ec);
		if (ec) continue;
		int64_t s_mtime      = FileTimeTypeToTimeT(mtime);
		file_cache[abs_path] = s_mtime;
	}

	for (const auto& abs_path : compress_tasks) {
		std::error_code ec;
		auto            mtime = std::filesystem::last_write_time(abs_path, ec);
		if (ec) continue;
		int64_t s_mtime      = FileTimeTypeToTimeT(mtime);
		file_cache[abs_path] = s_mtime;
	}

	json cache_out_j;
	for (const auto& [k, v] : file_cache) {
		cache_out_j[k] = v;
	}
	std::ofstream cache_out(cache_path);
	cache_out << cache_out_j.dump();
	cache_out.close();
}