#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"

typedef glm::fvec3 Color;

class Texture {
	public:
		virtual Color color(float u, float v, const glm::fvec3 &p) const = 0;
};

#endif
