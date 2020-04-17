#ifndef SUPERSDL_LOGGABLE_HPP
#define SUPERSDL_LOGGABLE_HPP

#include <memory>
#include <spdlog/logger.h>

namespace sps {

class CLoggable {
  private:
	std::shared_ptr<spdlog::logger> m_Logger;

  protected:
	auto Log() const { return m_Logger; }

  public:
	CLoggable(const char *pName);
};

} // namespace sps

#endif
