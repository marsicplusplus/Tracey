#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "defs.hpp"
#include "ray.hpp"

#include <memory>
#include <random>

typedef glm::dvec3 Color;
struct HitRecord;

class Material {
	public:
		virtual bool scatter(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, std::mt19937 &gen) const = 0;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
