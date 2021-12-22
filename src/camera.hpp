#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include "glm/vec3.hpp"
#include "ray.hpp"

#include <memory>

enum class CameraType {
	normal, 
	barrel,
	fisheye
};

class Camera{
	public:
		Camera(glm::fvec3 origin, glm::fvec3 dir, glm::fvec3 up, float fov);
		Ray generateCameraRay(float u, float v);

		void setPosition(glm::fvec3 pos);
		void setDirection(glm::fvec3 dir, bool update = true);
		void setFOV(float fov);
		void setCameraType(CameraType type);
		void setSensitivity(float sensitivity);
		void setFisheyeAngle(float angle);
		void setDistortionCoefficients(float k1, float k2);

		inline float getFOV() const { return this->fov; };
		inline float getSensitivity() const { return this->sensitivity; };
		inline glm::fvec3 getPosition() const { return position; }
		inline glm::fvec3 getDirection() const { return direction; }
		bool update(float dt, bool forceUpdate = false);

		glm::fvec2 Distort(float u, float v);
		glm::fvec3 Fisheye(float u, float v);
	private:
		void updateVectors();

		CameraType cameraType;

		glm::fvec3 up;
		glm::fvec3 right;
		glm::fvec3 position;
		glm::fvec3 direction;
		glm::fvec3 horizontal;
		glm::fvec3 vertical;
		glm::fvec3 llCorner;

		glm::mat3x3 cameraMatrix;

		float fov;
		float aspectRatio;
		float viewportWidth;
		float viewportHeight;
		float sensitivity;
		float k1;
		float k2;
		float fisheyeAngle;

		const float speed =1.0f;
		const float angleToRads = 0.01745329251994329576923690768489f;
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif
