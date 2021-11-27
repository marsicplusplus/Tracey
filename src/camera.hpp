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
		void setDirection(glm::dvec3 dir, bool update = true);
		void setFOV(double fov);

		inline double getFOV() const { return this->fov; };
		inline glm::dvec3 getPosition() const { return position; }
		inline glm::dvec3 getDirection() const { return direction; }
		bool update(double dt, bool forceUpdate = false);

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
		const double speed = 0.3;
		const double angleToRads = 0.01745329251994329576923690768489;
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif
