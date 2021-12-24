#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "defs.hpp"
#include "stb_image.h"
#include "glm/vec3.hpp"
#include <memory>
#include <string>

typedef glm::fvec3 Color;

class Texture {
	public:
		Texture(std::string _name) : name(_name) {img = nullptr;}
		virtual ~Texture() {
			stbi_image_free(img);
		}
		virtual Color color(float u, float v, const glm::fvec3 &p) const = 0;

		inline const std::string& getName() const {
			return name;
		}

	private:
		std::string name;
	protected:
		unsigned char *img;
};

typedef std::unique_ptr<Texture> TexturePtr;
#endif
