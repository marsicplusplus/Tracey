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
	MIRROR,
	EMISSIVE
};

class Material {
	public:
		Material(std::string name) : name{std::move(name)} {
			albedo = -1;
			bump = -1;
		}

		inline virtual bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay) const {
			return false;
		};

		inline virtual Materials getType() const {
			return Materials::NILL;
		}

		inline virtual bool refract(const Ray& in, const HitRecord &r, Ray &refractedRay) const {
			return false;
		}

		inline virtual void absorb(const Ray& in, const HitRecord& r, Color& attenuation) const {
			return;
		}

		inline const std::string& getName() const {
			return name;
		}

		inline int getAlbedoIdx() const {
			return albedo;
		}

	protected:
		int albedo;
		int bump;
		std::string name;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
