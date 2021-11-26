#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include "glm/vec3.hpp"
#include "ray.hpp"

#include <memory>

class Camera{
	public:
		Camera(glm::dvec3 origin, glm::dvec3 dir, glm::dvec3 up, double fov);
		Ray generateCameraRay(double u, double v);

		void setPosition(glm::dvec3 pos);
		inline glm::dvec3 getPosition() const {return position;}
		bool update(double dt);

	private:
		void updateVectors();

		glm::dvec3 up;
		glm::dvec3 position;
		glm::dvec3 direction;
		glm::dvec3 horizontal;
		glm::dvec3 vertical;
		glm::dvec3 llCorner;

		double fov;
		double aspectRatio;
		double viewportWidth;
		double viewportHeight;
		double sensitivity;
		const double speed = 0.8;
		const double angleToRads = 0.01745329251994329576923690768489;
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif
