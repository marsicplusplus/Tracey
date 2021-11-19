#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include "glm/vec3.hpp"
#include "ray.hpp"

class Camera{
	public:
		Camera(glm::dvec3 origin, glm::dvec3 dir, double fov);
		Ray generateCameraRay(int u, int v);

	private:
		glm::dvec3 position;
		glm::dvec3 front;
		glm::dvec3 horizontal;
		glm::dvec3 vertical;
		glm::dvec3 llCorner;

		double focalLength;
		double aspectRatio;
		double viewportWidth;
		double viewportHeight;
};

#endif
