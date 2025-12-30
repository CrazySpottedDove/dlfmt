#include "dl/ast_printer.h"
#include "dl/parser.h"
#include "dl/tokenizer.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <omp.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <system_error>
#include <unordered_map>
#include <vector>
using namespace dl;

const std::string VERSION = "0.0.9";

static void PrintHelp()
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
}

static void PrintVersion()
{
    printf("dlfmt version %s\n", VERSION.c_str());
}

static void FormatFile(const std::string& format_file)
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

    // tokenize
    Tokenizer<TokenizeMode::Format> tokenizer(std::move(content), format_file);

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

static void FormatDirectory(const std::string& format_directory)
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
            FormatFile(files[i]);
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


static void compressFile(const std::string& format_file)
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

    // tokenize
    Tokenizer<TokenizeMode::Compress> tokenizer(std::move(content), format_file);

    // parse
    Parser parser(tokenizer.getTokens(), format_file);

    // 打开输出
    std::ofstream out_file(format_file, std::ios::binary | std::ios::trunc);

    // 写入
    AstPrinter<AstPrintMode::Compress> printer(out_file);
    printer.PrintAst(parser.GetAstRoot());
    out_file.flush();
    out_file.close();
}

static void compressDirectory(const std::string& format_directory)
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
            compressFile(files[i]);
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
    std::string     abspath = std::filesystem::absolute(path).string();
    std::error_code ec;
    auto            mtime = std::filesystem::last_write_time(abspath, ec);
    if (ec) return true;
    int64_t s_mtime = FileTimeTypeToTimeT(mtime);
    auto    it      = file_cache.find(abspath);
    if (it != file_cache.end() && it->second == s_mtime) {
        return false;
    }
    return true;
}

static void ProcessJsonTask(const std::string& json_file)
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
                    exclude.push_back(std::filesystem::absolute(work_dir / ex.get<std::string>()));
            }
            // 遍历目录，收集所有lua文件
            for (const auto& entry :
                 std::filesystem::recursive_directory_iterator(task["directory"])) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    std::string abs_path =
                        std::filesystem::absolute(entry.path().string()).string();

                    // 文件没有变，不需要加入任务清单
                    if (!ShouldProcessFile(abs_path, file_cache)) {
                        continue;
                    }

                    bool is_excluded = false;
                    for (const auto& ex : exclude) {
                        if (abs_path.compare(0, ex.string().size(), ex.string()) == 0) {
                            is_excluded = true;
                            break;
                        }
                    }
                    // 文件路径被排除，不加入任务清单
                    if (is_excluded) {
                        continue;
                    }
                    compress_tasks.push_back(abs_path);
                }
            }
        }
        else if (task["type"] == "format") {
            std::vector<std::filesystem::path> exclude;
            if (task.contains("exclude")) {
                for (const auto& ex : task["exclude"]) {
                    exclude.push_back(std::filesystem::absolute(work_dir / ex.get<std::string>()));
                }
            }

            // 遍历目录，收集所有lua文件
            for (const auto& entry :
                 std::filesystem::recursive_directory_iterator(task["directory"])) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    std::string abs_path =
                        std::filesystem::absolute(entry.path().string()).string();

                    // 文件没有变，不需要加入任务清单
                    if (!ShouldProcessFile(abs_path, file_cache)) {
                        continue;
                    }

                    bool is_excluded = false;

                    for (const auto& ex : exclude) {
                        if (abs_path.compare(0, ex.string().size(), ex.string()) == 0) {
                            is_excluded = true;
                            break;
                        }
                    }
                    // 文件路径被排除，不加入任务清单
                    if (is_excluded) {
                        continue;
                    }
                    format_tasks.push_back(abs_path);
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
        FormatFile(abs_path);
    }

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(compress_tasks.size()); ++i) {
        const auto& abs_path = compress_tasks[i];
        compressFile(abs_path);
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
    cache_out << cache_out_j.dump(1, '\t');
    cache_out.close();
}

#define DLFMT_WORK_MODE_FORMAT_FILE 0
#define DLFMT_WORK_MODE_FORMAT_DIRECTORY 1
#define DLFMT_WORK_MODE_SHOW_HELP 2
#define DLFMT_WORK_MODE_SHOW_VERSION 3
#define DLFMT_WORK_MODE_COMPRESS_FILE 4
#define DLFMT_WORK_MODE_COMPRESS_DIRECTORY 5
#define DLFMT_WORK_MODE_JSON_TASK 6
int main(int argc, char* argv[])
{
    // 输出到控制台
    const auto console = spdlog::stdout_color_mt("console");
    console->set_pattern("[%^%l %s:%#%$] %v");
    spdlog::set_default_logger(console);
    std::string format_file;
    std::string format_directory;
    std::string task_file;
    int         work_mode = DLFMT_WORK_MODE_SHOW_HELP;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            work_mode = DLFMT_WORK_MODE_SHOW_HELP;
            break;
        }
        else if (arg == "--version") {
            work_mode = DLFMT_WORK_MODE_SHOW_VERSION;
            break;
        }
        else if (arg == "--format-file") {
            if (i + 1 < argc) {
                format_file = argv[i + 1];
                work_mode   = DLFMT_WORK_MODE_FORMAT_FILE;
                break;
            }
            else {
                SPDLOG_ERROR("No file specified after --format-file");
                return 1;
            }
        }
        else if (arg == "--format-directory") {
            if (i + 1 < argc) {
                format_directory = argv[i + 1];
                work_mode        = DLFMT_WORK_MODE_FORMAT_DIRECTORY;
                break;
            }
            else {
                SPDLOG_ERROR("No directory specified after --format-directory");
                return 1;
            }
        }
        else if (arg == "--compress-file") {
            if (i + 1 < argc) {
                format_file = argv[i + 1];
                work_mode   = DLFMT_WORK_MODE_COMPRESS_FILE;
                break;
            }
            else {
                SPDLOG_ERROR("No file specified after --compress-file");
                return 1;
            }
        }
        else if (arg == "--compress-directory") {
            if (i + 1 < argc) {
                format_directory = argv[i + 1];
                work_mode        = DLFMT_WORK_MODE_COMPRESS_DIRECTORY;
                break;
            }
            else {
                SPDLOG_ERROR("No directory specified after --compress-directory");
                return 1;
            }
        }
        else if (arg == "--json-task") {
            if (i + 1 < argc) {
                task_file = argv[i + 1];
                work_mode = DLFMT_WORK_MODE_JSON_TASK;
                break;
            }
            else {
                SPDLOG_ERROR("No json file specified after --json-task");
                return 1;
            }
        }
    }

    switch (work_mode) {
    case DLFMT_WORK_MODE_SHOW_HELP: PrintHelp(); return 0;
    case DLFMT_WORK_MODE_SHOW_VERSION: PrintVersion(); return 0;
    case DLFMT_WORK_MODE_FORMAT_DIRECTORY:
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        FormatDirectory(format_directory);
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        SPDLOG_INFO("Formatted directory '{}' in {} ms.", format_directory, duration);
        return 0;
    }
    case DLFMT_WORK_MODE_FORMAT_FILE:
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        FormatFile(format_file);
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        SPDLOG_INFO("Formatted file '{}' in {} ms.", format_file, duration);
        return 0;
    }
    case DLFMT_WORK_MODE_COMPRESS_DIRECTORY:
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        compressDirectory(format_directory);
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        SPDLOG_INFO("Compressed directory '{}' in {} ms.", format_directory, duration);
        return 0;
    }
    case DLFMT_WORK_MODE_COMPRESS_FILE:
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        compressFile(format_file);
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        SPDLOG_INFO("Compressed file '{}' in {} ms.", format_file, duration);
        return 0;
    }
    case DLFMT_WORK_MODE_JSON_TASK:
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        ProcessJsonTask(task_file);
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        SPDLOG_INFO("Processed json task '{}' in {} ms.", task_file, duration);
        return 0;
    }
    default: SPDLOG_ERROR("Unknown work mode."); return 1;
    }

    return 0;
}