#include "camera.hpp"
#include "defs.hpp"

Camera::Camera(glm::dvec3 origin, glm::dvec3 dir, glm::dvec3 up, double fov) : position{origin}, direction{dir}, up{up}, fov{glm::radians(fov)} {
	updateVectors();
}

Ray Camera::generateCameraRay(double u, double v){
	glm::dvec3 dir = llCorner + u*horizontal + v*vertical - position;
	Ray ray(position, dir);
	return ray;
}

void Camera::setPosition(glm::dvec3 pos) {
	this->position = pos;
	this->updateVectors();
}

void Camera::updateVectors(){
	this->aspectRatio = static_cast<double>(W_WIDTH)/static_cast<double>(W_HEIGHT);
	double h = tan(fov/2.0);
	this->viewportHeight = 2.0 * h;
	this->viewportWidth = this->aspectRatio * this->viewportHeight;
	auto w = glm::normalize(position - direction);
	auto u = glm::normalize(glm::cross(up, w));
	auto v = glm::cross(w, u);
	horizontal = viewportWidth * u;
	vertical = viewportHeight * v;
	this->llCorner = this->position - this->horizontal/2.0 - this->vertical/2.0 - w;
}
