#include "camera.hpp"
#include "options_manager.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "defs.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(glm::fvec3 origin, glm::fvec3 dir, glm::fvec3 up, float fov) : position{origin}, direction{dir}, up{up}, fov{glm::radians(fov)}, cameraType(CameraType::normal), fisheyeAngle(glm::radians(90.0f)){
	this->aspectRatio = static_cast<float>(OptionsMap::Instance()->getOption(Options::W_WIDTH))/static_cast<float>(OptionsMap::Instance()->getOption(Options::W_HEIGHT));
	this->sensitivity = 5;
	this->k1 = 1;
	this->k2 = 1;
	cameraMatrix = glm::mat3x3(1);
	updateVectors();
}

Ray Camera::generateCameraRay(float u, float v) {

	glm::fvec3 rayDir;

	switch (this->cameraType) {
		case CameraType::fisheye: {
			auto xyz = Fisheye(u, v);
			if (xyz == glm::fvec3(0, 0, 0)) {
				rayDir = xyz;
			} else {
				// Transform directional vector from x, y, z axes to axes of our camera
				rayDir = this->cameraMatrix * xyz;
			}
			break;
		}
		case CameraType::barrel: {
			auto uv = Distort(u, v);
			u = uv.x;
			v = uv.y;
		}
		case CameraType::normal:
		default: {
			rayDir = this->llCorner + u * this->horizontal + v * this->vertical - this->position;
			break;
		}
	}

	Ray ray(this->position, rayDir);
	return ray;
}

void Camera::setPosition(glm::fvec3 pos) {
	this->position = pos;
	this->updateVectors();
}

void Camera::setDirection(glm::fvec3 dir, bool update) {
	if (dir != glm::fvec3(0, 0, 0))
		this->direction = dir;
	else
		this->direction = glm::fvec3(0, 0, -1);

	if (update) {
		this->updateVectors();
	}
}

void Camera::setFOV(float fov) {
	this->fov = glm::radians(fov);
	this->updateVectors();
};

void Camera::setCameraType(CameraType type) {
	this->cameraType = type;
};

void Camera::setSensitivity(float sensitivity) {
	this->sensitivity = sensitivity;
};

void Camera::setFisheyeAngle(float angle) {
	this->fisheyeAngle = angle;
};

void Camera::setDistortionCoefficients(float k1, float k2){
	this->k1 = k1;
	this->k2 = k2;
}

void Camera::updateVectors() {
	float h = tan(this->fov / 2.0f);
	this->viewportHeight = 2.0f * h;
	this->viewportWidth = this->aspectRatio * this->viewportHeight;

	// Redefine Up and Right as we allow for camera rotation
	this->direction = glm::normalize(this->direction);
	auto w = this->direction;
	this->right = glm::cross(w, glm::fvec3(0, 1, 0));
	if (this->right == glm::fvec3(0, 0, 0))
		this->right = glm::fvec3(0, 0, 1);
	this->up = glm::cross(this->right, w);

	this->cameraMatrix = glm::mat3x3(this->right, this->up, this->direction);

	auto u = glm::normalize(glm::cross(w, this->up));
	auto v = glm::cross(u, w);
	this->horizontal = this->viewportWidth * u;
	this->vertical = this->viewportHeight * v;
	this->llCorner = this->position - this->horizontal/2.0f - this->vertical/2.0f + w;
}

bool Camera::update(float dt, bool forceUpdate) {
	bool updated = forceUpdate;
	auto inputManager = InputManager::Instance();
	float scroll = inputManager->getScrollState();
	if (scroll != 0) {
		this->fov -= 0.5f * ((scroll > 0) ? dt : -dt);
		this->fov = std::clamp(this->fov, glm::radians(25.0f), glm::radians(120.0f));
		InputManager::Instance()->scrollState(0.0f);
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_MOUSE_BUTTON_RIGHT)) {
		MouseState ms = inputManager->getMouseState();
		if (ms.moved) {
			float xMovement = ms.dx * this->angleToRads * (.05f * this->sensitivity);
			float yMovement = ms.dy * this->angleToRads * (.05f * this->sensitivity);
			this->direction = glm::rotate(this->direction, -yMovement, this->right);
			this->direction = glm::rotate(this->direction, -xMovement, this->up);
			updated = true;
		}
	}

	const float change = this->speed * dt;
	if (inputManager->isKeyDown(GLFW_KEY_W)) {
		this->position += this->direction * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_S)) {
		this->position -= this->direction * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_A)) {
		this->position -= this->right * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_D)) {
		this->position += this->right * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_Q)) {
		this->position += this->up * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_E)) {
		this->position -= this->up * change;
		updated = true;
	}

	if (updated) {
		updateVectors();
	}

	return updated;
}

glm::fvec2 Camera::Distort(float u, float v)
{
	// Using The Division Model from A.W. Fitzgibbon 
	// "Simultaneous linear estimation of multiple view geometry and lens distortion"
	auto uv = glm::vec2(u, v);
	// Bring uv from 0 - +1 to -1 - +1
	uv *= 2.0;
	uv -= 1.0;

	// Calculate angle of uv with x axis
	float phi = glm::atan(uv.y, uv.x);
	float undistorted = glm::length(uv);

	float distorted = undistorted * (1 + this->k1 * pow(undistorted, 2) + this->k2 * pow(undistorted, 4));
	uv.x = distorted * cos(phi);
	uv.y = distorted * sin(phi);

	// Bring uv from -1 - +1 to 0 - +1
	uv += 1;
	uv *= 0.5f;
	return uv;
}

glm::fvec3 Camera::Fisheye(float u, float v)
{
	// Adapted from http://paulbourke.net/dome/fisheye/
	// Anything uv with a radius larger than 1 will be black pixel (Left)
	// Imagine we create a sphere at the origin of radius 1
	// We constuct xyz which is uv rotated about the x axis by theta (max of 180) (right)
	//
	//                 ^y                
	//                |                                              ^y
	//               1|                 			                | 
	//          * * * * * * *    black        		                | 
	//        * *     |   uv   *         			          * * * * 
	//      * *       |   / |    *       			        * * xyz | 
	//      *         |  /  |     *       			      * *   \   | 
	//     *          | /phi|       *       		      *      \  | 
	//(-1)* --------- * ----------*-1---->x			     *        \ | 
	//     *          |          *       			z<--* ---------\|
	//      *         |         *       			     *          | 
	//      * *       |       * *       			      *         | 
	//        * *     |     * *         			      * *       | 
	//          * * * * * * *   black        		        * *     | 
	//                |                 			          * * * * 
	//                |                 			                | 
	//                |                 			                | 
	//

	
	// Bring u and v from 0 - 1 to -1 - +1
	u *= 2;
	v *= 2;
	u -= 1;
	v -= 1;

	// Adjust for aspect ratio
	if (this->viewportWidth > this->viewportHeight) {
		u *= aspectRatio;
	} else if (this->viewportHeight > this->viewportWidth) {
		v /= aspectRatio;
	}

	auto uv = glm::vec2(u, v);


	float radius = glm::length(uv);
	if (radius > 1) {
		return glm::vec3(0, 0, 0);
	}

	// Calculate angle of uv with x axis
	float phi = glm::atan(uv.y, uv.x);

	// Calculate angle of uv with z axis
	float theta = radius * this->fisheyeAngle;

	// Determine directional vector xyz
	float x = sin(theta) * cos(phi);
	float y = sin(theta) * sin(phi);
	float z = cos(theta);

	return glm::vec3(x, y, z);
}
