#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#include <SuperSDL/engine.hpp>

namespace sps {

CEngine::CEngine() : CLoggable("engine"), m_AppConfigPath() {
}

void CEngine::init(const char *pOrgName, const char *pGameName) {
	m_pOrgName = pOrgName;
	m_pGameName = pGameName;

	SDL_Init(SDL_INIT_EVERYTHING);
	Log()->info("Initialized engine.");

	char* path = SDL_GetPrefPath(pOrgName, pGameName); 
	m_AppConfigPath = path;
	SDL_free(path);

	Log()->info("Config path: {}", m_AppConfigPath);
}

void CEngine::quit() {
	Log()->info("Stopping engine.");
	SDL_Quit();
}

} // namespace sps
