#ifndef SUPERSDL_ENGINE_HPP
#define SUPERSDL_ENGINE_HPP

#include "loggable.hpp"
#include <spdlog/logger.h>
#include <SDL.h>
#include <string>
#include "util.hpp"

namespace sps {

class CEngine : CLoggable {
	private:
		std::string m_AppConfigPath;
		const char *m_pOrgName;
		const char *m_pGameName;

  public:
	CEngine();
	void init(const char *pOrgName, const char *pGameName);
	void quit();

	const char *get_org_name() { return m_pOrgName; }
	const char *get_game_name() { return m_pGameName; }
};

} // namespace sps

#endif
