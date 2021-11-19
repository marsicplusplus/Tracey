#include "camera.hpp"
#include "defs.hpp"

Camera::Camera(glm::dvec3 origin, glm::dvec3 dir, double focal) : position{origin}, front{dir}, focalLength{focal} {
	this->aspectRatio = static_cast<double>(W_WIDTH)/static_cast<double>(W_HEIGHT);
	this->viewportHeight = 2.0;
	this->viewportWidth = this->aspectRatio * this->viewportHeight;
	horizontal = glm::dvec3(viewportWidth, 0, 0);
	vertical = glm::dvec3(0, viewportHeight, 0);
	llCorner = origin - horizontal/2.0 - vertical/2.0 - glm::dvec3(0, 0, focalLength);
}

Ray Camera::generateCameraRay(int x, int y){
	double u = static_cast<double>(x) / static_cast<double>(W_WIDTH - 1);
	double v = static_cast<double>(y) / static_cast<double>(W_HEIGHT - 1);
	glm::dvec3 dir = llCorner + u*horizontal + v*vertical - position;
	return Ray(position, dir);
}
