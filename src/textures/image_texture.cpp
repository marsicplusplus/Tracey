#include "textures/image_texture.hpp"

ImageTexture::ImageTexture(std::string name, std::string fp) : Texture(name){
	int channels = bpp;
	img = stbi_load(fp.c_str(), &width, &height, &channels, channels);
	if (!img) {
		std::cerr << "ERROR: Could not load texture image file '" << fp << "'.\n";
		width = height = 0;
	}
	//if(((width & (width-1)) != 0) || (height & (height-1)) != 0){
		//std::cerr << "ERROR: textures need to be power of 2'" << fp << "'.\n";
		//width = height = 0;
	//}
	slice = bpp * width;
}

ImageTexture::~ImageTexture(){}

Color ImageTexture::color(float u, float v, const glm::fvec3 &p) const{
	if(img == nullptr || width == 0 || height == 0) return Color(0.4,0.4,0.4);
	unsigned int i = static_cast<unsigned int>(u*width) % (width-1);
	unsigned int j = static_cast<unsigned int>(v*height) % (height-1);
	unsigned char *pixel = img + j * slice + i * bpp;
	return Color(pixel[0]/static_cast<float>(255.0f),pixel[1]/static_cast<float>(255.0f),pixel[2]/static_cast<float>(255.0f));
}
