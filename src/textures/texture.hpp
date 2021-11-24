#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"

typedef glm::dvec3 Color;

class Texture {
	public:
		virtual Color color(double u, double v, const glm::dvec3 &p) const = 0;
};

#endif
