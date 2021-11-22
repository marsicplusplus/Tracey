#ifndef __SOLID_COLOR_HPP__
#define __SOLID_COLOR_HPP__

#include "textures/texture.hpp"

class SolidColor : public Texture {
	public:
		SolidColor() : c{Color(0,0,0)} {}
		SolidColor(Color col) : c{col} {}
		SolidColor(double r, double g, double b) : c{Color(r, g, b)} {}
		Color color(double u, double v, const glm::dvec3 &p) const override {
			return c;
		}
	private:
		Color c;
};

#endif
