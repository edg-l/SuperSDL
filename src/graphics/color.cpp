#include <SuperSDL/color.hpp>

namespace sps {
CColor::CColor() : CColor(0) {}

CColor::CColor(int r, int g, int b, int a) {
	this->r = r / 255.f;
	this->g = g / 255.f;
	this->b = b / 255.f;
	this->a = a / 255.f;
}

CColor::CColor(int r, int g, int b) {
	this->r = r / 255.f;
	this->g = g / 255.f;
	this->b = b / 255.f;
	this->a = 1.f;
}

CColor::CColor(int all) : CColor(all, all, all, all) {}

CColor CColor::from_hex(int hex) {
	return CColor((hex & 0xFF0000) >> 16, (hex & 0xFF00) >> 8, (hex & 0xFF));
}

CColor CColor::operator+(const CColor &other) {
	return CColor(r + other.r, g + other.g, b + other.b, a + other.a);
}

CColor CColor::operator-(const CColor &other) {
	return CColor(r - other.r, g - other.g, b - other.b, a - other.a);
}

CColor CColor::operator*(const CColor &other) {
	return CColor(r * other.r, g * other.g, b * other.b, a * other.a);
}

} // namespace sps
