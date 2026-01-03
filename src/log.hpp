#pragma once

#include <hyprland/src/debug/log/Logger.hpp>

// Compatibility define for old code
#define LOG Log::DEBUG

template <typename... Args>
void hycov_log(Hyprutils::CLI::eLogLevel level, std::format_string<Args...> fmt, Args &&...args)
{
	auto msg = std::vformat(fmt.get(), std::make_format_args(args...));
	Log::logger->log(level, "[hycov] {}", msg);
}
