#include <SuperSDL/loggable.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace sps {

CLoggable::CLoggable(const char *pName) {
	m_Logger = spdlog::stdout_color_mt(pName);
#ifndef NDEBUG
	m_Logger->set_level(spdlog::level::debug);
#endif
}

} // namespace sps
