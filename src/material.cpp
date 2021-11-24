#include "material.hpp"

#include "defs.hpp"
#include "ray.hpp"

Color Material::getAlbedo(HitRecord &rec) const {
	return albedo->color(rec.u, rec.v, rec.p);
}
inline Materials Material::getType() const {
	return materialType;
}

