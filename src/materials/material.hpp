#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "ray.hpp"

#include <memory>
#include <random>

struct HitRecord;
typedef glm::dvec3 Color;

enum class Materials {
	NILL,
	DIFFUSE,
	DIELECTRIC,
	MIRROR
};

class Material {
	public:
		inline virtual bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const {
			return false;
		};
		inline virtual Materials getType() const {
			return Materials::NILL;
		}
		inline virtual bool refract(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const {
			return false;
		}
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
