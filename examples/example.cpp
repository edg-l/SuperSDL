#include <SuperSDL/supersdl.hpp>

class CMyGame : public sps::CGame {
  protected:
	virtual void onLoad() {}
	virtual void onRender(double delta) {}
	virtual void onUpdate(double delta) {}
  public:
	CMyGame() : sps::CGame("Ryozuki", "SuperSDLGame"){}
};

int main() {
	CMyGame Game;
	Game.start();
	return 0;
}
