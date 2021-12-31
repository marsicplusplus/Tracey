#include "lights/point_light.hpp"

PointLight::PointLight(glm::fvec3 pos, glm::fvec3 i) : 
	LightObject((int)LightType::LIGHT_POINT, i){
		transform.translate(pos);
	}

inline Ray PointLight::getRay(const HitRecord &rec, float &tMax) const {
	tMax = glm::distance(this->transform.getTranslation(), rec.p);
	auto dir = this->transform.getTranslation() - rec.p;
	return Ray(rec.p + 0.001f * dir, dir);
}

inline Color PointLight::attenuate(Color color, const glm::fvec3 &p) {
	return color * 1.0f / glm::distance(this->transform.getTranslation(), p);
}
