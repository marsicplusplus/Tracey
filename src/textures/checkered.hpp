#ifndef __CHECKERED_HPP__
#define __CHECKERED_HPP__

#include "textures/texture.hpp"

class Checkered : public Texture {
	public:
		Checkered() : c1{Color(0,0,0)}, c2{Color(1,1,1)} {}
		Checkered(Color col1, Color col2) : c1{col1}, c2{col2} {}
		Checkered(double r1, double g1, double b1, double r2, double g2, double b2) : c1{Color(r1, g1, b1)}, c2{Color(r2, g2, b2)} {}
		Color color(double u, double v, const glm::dvec3 &p) const override {
			auto sines = sin(10*p.x)*sin(10*p.y)*sin(10*p.z);
			return (sines < 0) ? c1 : c2;
		}
	private:
		Color c1;
		Color c2;
};

#endif
