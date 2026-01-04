
#include <nlohmann/json.hpp>
#include <omp.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

enum class dlfmt_mode{
    show_help,
    show_version,
    format_file,
    format_directory,
    compress_file,
    compress_directory,
    json_task
};

enum class dlfmt_param{
    auto_format,
    manual_format
};

void ShowHelp();

void ShowVersion();

void FormatFile(const std::string& format_file, dlfmt_param param);

void FormatDirectory(const std::string& format_directory, dlfmt_param param);

void CompressFile(const std::string& compress_file, [[maybe_unused]] dlfmt_param param);

void CompressDirectory(const std::string& compress_directory, [[maybe_unused]] dlfmt_param param);

void JsonTask(const std::string& json_file);