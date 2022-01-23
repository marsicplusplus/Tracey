#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "stb_image.h"
#include <memory>
#include <string>
#include <utility>

typedef glm::fvec3 Color;

enum class TextureType {
	TEXTURE_NIL			= 0,
	TEXTURE_IMAGE		= 0x00000001u,
	TEXTURE_SOLID		= 0x00000002u,
	TEXTURE_CHECKERED	= 0x00000004u,
};

struct CompactTexture{
	glm::vec4 color;
	glm::vec4 sec_color;
	unsigned int type;
	int idx;
	int w;
	int h;
	int slice;
	int bpp;
	float __padding[2];
};

class Texture {
	public:
		Texture(std::string _name) : name(std::move(_name)) {img = nullptr;}
		virtual ~Texture() {
			stbi_image_free(img);
		}
		virtual Color color(float u, float v, const glm::fvec3 &p) const = 0;

		inline const std::string& getName() const {
			return name;
		}
		virtual inline const TextureType getType() const {
			return TextureType::TEXTURE_NIL;
		}

		unsigned char *img;

	private:
		std::string name;
};

typedef std::shared_ptr<Texture> TexturePtr;
#endif
