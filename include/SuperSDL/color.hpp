#ifndef SUPERSDL_COLOR_HPP
#define SUPERSDL_COLOR_HPP

namespace sps {

class CColor {
  public:
	float r, g, b, a;

	CColor();
	CColor(int r, int g, int b);
	CColor(int r, int g, int b, int a);
	CColor(int all);

	// hex should be RGB
	static CColor from_hex(int hex);

	CColor operator+(const CColor &other);
	CColor operator-(const CColor &other);
	CColor operator*(const CColor &other);
};

const CColor COLOR_WHITE = CColor(255);
const CColor COLOR_BLACK = CColor(0);
const CColor COLOR_GRAY = CColor(128);
const CColor COLOR_RED = CColor(255, 0, 0);
const CColor COLOR_LIME = CColor(0, 255, 0);
const CColor COLOR_BLUE = CColor(0, 0, 255);
const CColor COLOR_YELLOW = CColor(255, 255, 0);
const CColor COLOR_CYAN = CColor(0, 255, 255);
const CColor COLOR_MAGENTA = CColor(255, 0, 255);
const CColor COLOR_SILVER = CColor(192);
const CColor COLOR_MAROON = CColor(128, 0, 0);
const CColor COLOR_GREEN = CColor(0, 128, 0);
const CColor COLOR_PURPLE = CColor(128, 0, 128);

} // namespace sps

#endif
