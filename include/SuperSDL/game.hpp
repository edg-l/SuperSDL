#ifndef SUPERSDL_GAME_HPP
#define SUPERSDL_GAME_HPP

#include "SuperSDL/renderer.hpp"
#include "engine.hpp"
#include "loggable.hpp"

namespace sps {

class CGame : CLoggable {
  private:
	CEngine m_Engine;
	CRenderer m_Renderer;
	bool m_Stop;
	const char *m_pOrgName;
	const char *m_pGameName;

  protected:
	void stop();
	virtual void on_load() = 0;
	virtual void on_update(double delta) = 0;
	virtual void on_render(double delta) = 0;

  public:
	CGame(const char *pOrgName, const char *pGameName);
	virtual ~CGame() {}
	void start();
};

} // namespace sps

#endif
