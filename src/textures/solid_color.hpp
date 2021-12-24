#ifndef __SOLID_COLOR_HPP__
#define __SOLID_COLOR_HPP__

#include "textures/texture.hpp"

class SolidColor : public Texture {
	public:
		SolidColor(std::string name) : Texture(name), c{Color(0,0,0)} {}
		SolidColor(std::string name, Color col) : Texture(name), c{col} {}
		SolidColor(std::string name, float r, float g, float b) : Texture(name), c{Color(r, g, b)} {}
		Color color(float u, float v, const glm::fvec3 &p) const override {
			return c;
		}
	private:
		Color c;
};

#endif
