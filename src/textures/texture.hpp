#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include <memory>
#include <string>

typedef glm::fvec3 Color;

class Texture {
	public:
		Texture(std::string _name) : name(_name) {}
		virtual Color color(float u, float v, const glm::fvec3 &p) const = 0;

		inline const std::string& getName() const {
			return name;
		}

	private:
		std::string name;
};

typedef std::shared_ptr<Texture> TexturePtr;
#endif
