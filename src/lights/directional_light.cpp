#include "lights/directional_light.hpp"

DirectionalLight::DirectionalLight(glm::fvec3 dir, Color i) :
	LightObject((int)LightType::LIGHT_DIRECTIONAL, i), direction(dir) {}

Ray DirectionalLight::getRay(const HitRecord &rec, float &tMax) const {
	tMax = INF;
	return Ray(rec.p + 0.001f*(-this->direction), -this->direction);
}
