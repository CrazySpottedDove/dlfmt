#pragma once
#include <chrono>

#include <string>
class Timer
{
public:
	Timer() = default;
	void setLabel(const std::string& lbl) { label = lbl; }
	void start() { start_time = std::chrono::high_resolution_clock::now(); }
	void stop() { end_time = std::chrono::high_resolution_clock::now(); }
	/**
	 * @brief print the elapsed time between start and stop in ms with label prefix
	 *
	 */
	void print()
	{
		const auto duration =
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        printf("%s in %ld ms\n", label.c_str(), duration);
	}

private:
	std::string                                                 label;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
};