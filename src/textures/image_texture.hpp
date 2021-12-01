#ifndef __IMAGE_TEXTURE_HPP__
#define __IMAGE_TEXTURE_HPP__

#include "textures/texture.hpp"
#include "stb_image.h"

#include <algorithm>
#include <iostream>

class ImageTexture : public Texture {
	public:
		ImageTexture(std::string filePath);
		~ImageTexture();
		Color color(double u, double v, const glm::dvec3 &p) const override;
	private:
		int width;
		int height;
		int slice;
		unsigned char *img;
		const static int bpp = 3;
};

inline ImageTexture::ImageTexture(std::string fp){
	int channels = bpp;
	img = stbi_load(fp.c_str(), &width, &height, &channels, channels);
	if (!img) {
		std::cerr << "ERROR: Could not load texture image file '" << fp << "'.\n";
		width = height = 0;
	}
	if(((width & (width-1)) != 0) || (height & (height-1)) != 0){
		std::cerr << "ERROR: textures need to be power of 2'" << fp << "'.\n";
		width = height = 0;
	}
	slice = bpp * width;
}

inline ImageTexture::~ImageTexture(){
	stbi_image_free(img);
}

inline Color ImageTexture::color(double u, double v, const glm::dvec3 &p) const{
	if(img == nullptr || width == 0 || height == 0) return Color(0.4,0.4,0.4);
	unsigned int i = static_cast<unsigned int>(u*width) & (width-1);
	unsigned int j = static_cast<unsigned int>((1-v)*height) & (height-1);
	unsigned char *pixel = img + j * slice + i * bpp;
	return Color(pixel[0]/static_cast<double>(255.0),pixel[1]/static_cast<double>(255.0),pixel[2]/static_cast<double>(255.0));
}

#endif
