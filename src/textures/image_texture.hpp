#ifndef __IMAGE_TEXTURE_HPP__
#define __IMAGE_TEXTURE_HPP__

#include "textures/texture.hpp"
#include "stb_image.h"

#include <algorithm>
#include <iostream>

class ImageTexture : public Texture {
	public:
		ImageTexture(std::string name, std::string filePath);
		~ImageTexture();
		Color color(float u, float v, const glm::fvec3 &p) const override;
	private:
		int width;
		int height;
		int slice;
		unsigned char *img;
		const static int bpp = 3;
};


#endif
