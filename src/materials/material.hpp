#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "ray.hpp"
#include "textures/texture.hpp"
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
		inline virtual bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, double &reflectance) const {
			return false;
		};

		inline virtual Materials getType() const {
			return Materials::NILL;
		}

		inline virtual bool refract(const Ray& in, const HitRecord &r, Ray &refractedRay, double &refractance) const {
			return false;
		}

		inline virtual void absorb(const Ray& in, const HitRecord& r, Color& attenuation) const {
			return;
		}

		inline virtual Color getMaterialColor(double u, double v, glm::dvec3 p) const {
			return albedo->color(u, v, p);
		}

	protected:
		std::shared_ptr<Texture> albedo;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
