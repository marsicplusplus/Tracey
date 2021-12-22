#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "ray.hpp"
#include "textures/texture.hpp"
#include <memory>
#include <random>
#include <string>
#include <utility>

struct HitRecord;
typedef glm::fvec3 Color;

enum class Materials {
	NILL,
	DIFFUSE,
	DIELECTRIC,
	MIRROR
};

class Material {
	public:
		Material(std::string name) : name{std::move(name)} {
			albedo = nullptr;
			bump = nullptr;
		}

		inline virtual bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, float &reflectance) const {
			return false;
		};

		inline virtual Materials getType() const {
			return Materials::NILL;
		}

		inline virtual bool refract(const Ray& in, const HitRecord &r, Ray &refractedRay, float &refractance) const {
			return false;
		}

		inline virtual void absorb(const Ray& in, const HitRecord& r, Color& attenuation) const {
			return;
		}

		inline virtual Color getMaterialColor(float u, float v, glm::fvec3 p) const {
			return albedo->color(u, v, p);
		}

		inline const std::string& getName() const {
			return name;
		}

	protected:
		std::shared_ptr<Texture> albedo;
		std::shared_ptr<Texture> bump;
		std::string name;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
