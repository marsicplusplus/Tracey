#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "ray.hpp"

#include <memory>
#include <random>

struct HitRecord;
typedef glm::dvec3 Color;

class Material {
	public:
		inline virtual bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const {
			return false;
		};
		inline virtual bool refract(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const {
			return false;
		}
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
