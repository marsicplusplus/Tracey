#include "material.hpp"

#include "defs.hpp"
#include "ray.hpp"

Color Material::getAlbedo(HitRecord &rec) const {
	return albedo->color(rec.u, rec.v, rec.p);
}
double Material::getSpecular() const {
	return specular;
}
double Material::getReflective() const {
	return reflect;
}
double Material::getRefractionIndex() const {
	return refraction;
}
