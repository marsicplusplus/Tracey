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
	NILL = 0,
	DIFFUSE = 0x00000001u,
	DIELECTRIC = 0x00000002u,
	MIRROR = 0x00000004u
};

struct CompactMaterial {
	unsigned int type;
	int albedoIdx;
	int bump;
	float reflectionIdx;
	glm::vec3 absorption;
	float ior;
};

class Material {
	public:
		Material(std::string name) : name{std::move(name)} {
			albedo = -1;
			bump = -1;
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

		inline const std::string& getName() const {
			return name;
		}

		inline int getAlbedoIdx() const {
			return albedo;
		}

		inline int getBump() const {
			return bump;
		}

	protected:
		int albedo;
		int bump;
		std::string name;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
