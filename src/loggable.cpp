#include <SuperSDL/loggable.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace sps {

CLoggable::CLoggable(const char *pName) {
	m_Logger = spdlog::stdout_color_mt(pName);
}

} // namespace sps
