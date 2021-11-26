#include "camera.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "defs.hpp"
#include <glm/gtx/transform.hpp>

Camera::Camera(glm::dvec3 origin, glm::dvec3 dir, glm::dvec3 up, double fov) : position{origin}, direction{dir}, up{up}, fov{glm::radians(fov)} {
	this->aspectRatio = static_cast<double>(W_WIDTH)/static_cast<double>(W_HEIGHT);
	this->sensitivity = 0.25;
	updateVectors();
}

Ray Camera::generateCameraRay(double u, double v){
	glm::dvec3 rayDir = this->llCorner + u * this->horizontal + v * this->vertical - this->position;
	Ray ray(this->position, rayDir);
	return ray;
}

void Camera::setPosition(glm::dvec3 pos) {
	this->position = pos;
	this->updateVectors();
}

void Camera::updateVectors(){
	double h = tan(this->fov/2.0);
	this->viewportHeight = 2.0 * h;
	this->viewportWidth = this->aspectRatio * this->viewportHeight;
	auto w = glm::normalize(this->direction);
	auto u = glm::normalize(glm::cross(w, this->up));
	auto v = glm::cross(u, w);
	this->horizontal = this->viewportWidth * u;
	this->vertical = this->viewportHeight * v;
	this->llCorner = this->position - this->horizontal/2.0 - this->vertical/2.0 + w;
}

bool Camera::update(double dt) {
	bool updated = false;
	auto inputManager = InputManager::Instance();
	float scroll = inputManager->getScrollState();
	if (scroll != 0) {
		this->position += this->direction * ((scroll > 0) ? dt : -dt);
		InputManager::Instance()->scrollState(0.0);
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_MOUSE_BUTTON_RIGHT)) {
		MouseState ms = inputManager->getMouseState();
		if (ms.moved) {
			double xMovement = ms.dx * this->angleToRads * this->sensitivity;
			double yMovement = ms.dy * this->angleToRads * this->sensitivity;
			this->direction = glm::mat3(glm::rotate(-xMovement, this->up)) * this->direction;
			this->direction = glm::mat3(glm::rotate(-yMovement, glm::cross(direction, this->up))) * this->direction;
			updated = true;
		}
	}

	const double change = this->speed * dt;
	if (inputManager->isKeyDown(GLFW_KEY_W)) {
		this->position += this->direction * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_S)) {
		this->position -= this->direction * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_A)) {
		this->position -= glm::normalize(glm::cross(this->direction, this->up)) * change;
		updated = true;
	}

	if (inputManager->isKeyDown(GLFW_KEY_D)) {
		this->position += glm::normalize(glm::cross(this->direction, this->up)) * change;
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
