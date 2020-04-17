#include <SuperSDL/supersdl.hpp>

class CMyGame : public sps::CGame {
  protected:
	virtual void on_load() {}
	virtual void on_render(double delta) {}
	virtual void on_update(double delta) {}
  public:
	CMyGame() : sps::CGame("Ryozuki", "SuperSDLGame"){}
};

int main() {
	CMyGame Game;
	Game.start();
	return 0;
}
