#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

int main()
{
    // 输出到控制台
    spdlog::info("Hello, spdlog!");

    // 格式化输出
    int value = 42;
    spdlog::warn("The answer is {}", value);

    // 输出到文件
    auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/output.txt");
    file_logger->info("Logging to a file!");

    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("This is a debug message");

    return 0;
}