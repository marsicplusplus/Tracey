#include "lights/spot_light.hpp"

SpotLight::SpotLight(glm::fvec3 pos, glm::fvec3 dir, float cutoff, glm::fvec3 i) : 
	LightObject((int)LightType::LIGHT_SPOT, i), direction{dir}, cutoffAngle{cutoff} {
	this->transform.translate(pos);
}
Ray SpotLight::getRay(const HitRecord& rec, float& tMax) const {
	tMax = glm::distance(this->transform.getTranslation(), rec.p);
	auto dir = this->transform.getTranslation() - rec.p;
	auto hitToLight = Ray(rec.p + 0.001f * dir, dir);

	auto angle = std::acos(glm::dot(direction, glm::normalize(rec.p - this->transform.getTranslation())));
	if (angle > cutoffAngle) {
		return Ray(rec.p, glm::fvec3(0, 0, 0));
	} else {
		return hitToLight;
	}
}

Color SpotLight::attenuate(Color color, const glm::fvec3& p){
	return color * 1.0f / glm::distance(this->transform.getTranslation(), p);
}
