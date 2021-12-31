#include "lights/ambient_light.hpp"

AmbientLight::AmbientLight(Color i) : LightObject((int)LightType::LIGHT_AMBIENT, i) {}
Ray AmbientLight::getRay(const HitRecord &rec, float &tMax) const {
	tMax = 0.0001f;
	Ray ray(rec.p + 0.001f * rec.normal, rec.normal);
	return ray;
}

Color AmbientLight::getLight(const HitRecord &rec, Ray& ray) const {
	return this->intensity;
}

