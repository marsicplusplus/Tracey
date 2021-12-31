#include "lights/area_light.hpp"

AreaLight::AreaLight(glm::fvec3 pos, glm::fvec3 i, const std::shared_ptr<Hittable> &aLight) :
	LightObject((int)LightType::LIGHT_AREA, i), aLight(aLight), area(aLight->area()) {}

