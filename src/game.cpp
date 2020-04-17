#include <SuperSDL/game.hpp>

namespace sps {

CGame::CGame(const char *pOrgName, const char *pGameName) 
	: CLoggable("game"), m_Renderer(&m_Engine) {
	m_Stop = false;
	m_pOrgName = pOrgName;
	m_pGameName = pGameName;
}

void CGame::start() {
	Log()->info("Starting game.");
	m_Engine.init(m_pOrgName, m_pGameName);
	m_Renderer.init();

	stop();
	while (!m_Stop) {}

	m_Engine.quit();
}

void CGame::stop() {
	m_Stop = true;
}

} // namespace sps
