#ifndef __CHECKERED_HPP__
#define __CHECKERED_HPP__

#include "textures/texture.hpp"

class Checkered : public Texture {
	public:
		Checkered(std::string name) : Texture(name), c1{Color(0,0,0)}, c2{Color(1,1,1)} {}
		Checkered(std::string name, Color col1, Color col2) : Texture(name), c1{col1}, c2{col2} {}
		Checkered(std::string name, float r1, float g1, float b1, float r2, float g2, float b2) : Texture(name), c1{Color(r1, g1, b1)}, c2{Color(r2, g2, b2)} {}
		Color color(float u, float v, const glm::fvec3 &p) const override {
			auto sines = sin(10.0f*p.x)*sin(10.0f*p.y)*sin(10.0f*p.z);
			return (sines < 0.0f) ? c1 : c2;
		}
	private:
		Color c1;
		Color c2;
};

#endif
