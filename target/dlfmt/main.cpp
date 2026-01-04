#include "dl/timer.h"
#include "dlfmt_core.h"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
	const auto console = spdlog::stdout_color_mt("console");
	console->set_pattern("[%^%l %s:%#%$] %v");
	spdlog::set_default_logger(console);
	dlfmt_mode  work_mode  = dlfmt_mode::show_help;
	dlfmt_param work_param = dlfmt_param::auto_format;
	std::string file_or_directory;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--help") {
			work_mode = dlfmt_mode::show_help;
		}
		else if (arg == "--version") {
			work_mode = dlfmt_mode::show_version;
		}
		else if (arg == "--format-file") {
			if (i + 1 < argc) {
				file_or_directory = argv[++i];
				work_mode         = dlfmt_mode::format_file;
			}
			else {
				SPDLOG_ERROR("No file specified after --format-file");
				return 1;
			}
		}
		else if (arg == "--format-directory") {
			if (i + 1 < argc) {
				file_or_directory = argv[++i];
				work_mode         = dlfmt_mode::format_directory;
			}
			else {
				SPDLOG_ERROR("No directory specified after --format-directory");
				return 1;
			}
		}
		else if (arg == "--compress-file") {
			if (i + 1 < argc) {
				file_or_directory = argv[++i];
				work_mode         = dlfmt_mode::compress_file;
			}
			else {
				SPDLOG_ERROR("No file specified after --compress-file");
				return 1;
			}
		}
		else if (arg == "--compress-directory") {
			if (i + 1 < argc) {
				file_or_directory = argv[++i];
				work_mode         = dlfmt_mode::compress_directory;
			}
			else {
				SPDLOG_ERROR("No directory specified after --compress-directory");
				return 1;
			}
		}
		else if (arg == "--json-task") {
			if (i + 1 < argc) {
				file_or_directory = argv[++i];
				work_mode         = dlfmt_mode::json_task;
			}
			else {
				SPDLOG_ERROR("No json file specified after --json-task");
				return 1;
			}
		}
		else if (arg == "--param") {
			if (i + 1 < argc) {
				std::string param = argv[++i];
				if (param == "auto") {
					work_param = dlfmt_param::auto_format;
				}
				else if (param == "manual") {
					work_param = dlfmt_param::manual_format;
				}
				else {
					SPDLOG_ERROR("Unknown param: {}", param);
					return 1;
				}
			}
		}
	}

    if(work_mode == dlfmt_mode::show_help){
        ShowHelp();
        return 0;
    }

    if(work_mode == dlfmt_mode::show_version){
        ShowVersion();
        return 0;
    }

    Timer timer;
    timer.start();
    switch (work_mode) {
        case dlfmt_mode::format_file:{
			timer.setLabel(fmt::format("Formatted file '{}'", file_or_directory));
			FormatFile(file_or_directory, work_param);
			break;
		}
        case dlfmt_mode::format_directory:{
            timer.setLabel(fmt::format("Formatted directory '{}'", file_or_directory));
            FormatDirectory(file_or_directory, work_param);
            break;
        }
        case dlfmt_mode::compress_file:{
            timer.setLabel(fmt::format("Compressed file '{}'", file_or_directory));
            CompressFile(file_or_directory, work_param);
            break;
        }
        case dlfmt_mode::compress_directory:{
            timer.setLabel(fmt::format("Compressed directory '{}'", file_or_directory));
            CompressDirectory(file_or_directory, work_param);
            break;
        }
        case dlfmt_mode::json_task:{
            timer.setLabel(fmt::format("Processed json task file '{}'", file_or_directory));
            JsonTask(file_or_directory);
            break;
        }
        default:
            SPDLOG_ERROR("No valid work mode specified.");
            return 1;
    }
    timer.stop();
    timer.print();
    return 0;
}