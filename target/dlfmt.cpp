#include "dl/ast_tools.h"
#include "dl/macros.h"
#include "dl/parser.h"
#include "dl/tokenizer.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
using namespace dl;

const std::string VERSION = "0.0.2";

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

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // tokenize
    Tokenizer tokenizer(std::move(content), format_file, WORK_MODE_FORMAT);
    // parse
    Parser parser(tokenizer.getTokens(), format_file);

    // 打开输出
    std::ofstream out_file(format_file, std::ios::binary | std::ios::trunc);

    // 写入
    AstTools::PrintAst(parser.GetAstRoot(), tokenizer.getCommentTokens(), out_file);
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

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // tokenize
    Tokenizer tokenizer(std::move(content), format_file, WORK_MODE_COMPILE);
    // parse
    Parser parser(tokenizer.getTokens(), format_file);

    // 打开输出
    std::ofstream out_file(format_file, std::ios::binary | std::ios::trunc);

    // 写入
    AstTools::PrintAst(parser.GetAstRoot(), out_file);
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

#define DLFMT_WORK_MODE_FORMAT_FILE 0
#define DLFMT_WORK_MODE_FORMAT_DIRECTORY 1
#define DLFMT_WORK_MODE_SHOW_HELP 2
#define DLFMT_WORK_MODE_SHOW_VERSION 3
#define DLFMT_WORK_MODE_COMPRESS_FILE 4
#define DLFMT_WORK_MODE_COMPRESS_DIRECTORY 5

int main(int argc, char* argv[])
{
    // 输出到控制台
    const auto console = spdlog::stdout_color_mt("console");
    console->set_pattern("[%^%l %s:%#%$] %v");
    spdlog::set_default_logger(console);
    std::string format_file;
    std::string format_directory;
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
    default: SPDLOG_ERROR("Unknown work mode."); return 1;
    }

    return 0;
}