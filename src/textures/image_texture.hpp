#ifndef __IMAGE_TEXTURE_HPP__
#define __IMAGE_TEXTURE_HPP__

#include "textures/texture.hpp"

#include <algorithm>
#include <iostream>

class ImageTexture : public Texture {
	public:
		ImageTexture(std::string name, std::string filePath);
		~ImageTexture() override;
		Color color(float u, float v, const glm::fvec3 &p) const override;

		const TextureType getType() const override;

		int width;
		int height;
		int slice;
		const static int bpp = 3;
};


#endif
