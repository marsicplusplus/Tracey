#ifndef __MATERIAL_MIRROR_HPP__
#define __MATERIAL_MIRROR_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class MirrorMaterial : public Material {

	public: 
		MirrorMaterial(std::string name, int albedoIdx, float _reflective = 1.0f) : Material{name} {
			albedo = albedoIdx;
			reflectionIdx = _reflective;
		}

		inline Materials getType() const override { return Materials::MIRROR; }

		inline bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, float &reflectance) const override {
			reflectance = reflectionIdx;
			auto reflectedDir = glm::reflect(in.getDirection(), r.normal);
			reflectedRay = Ray(r.p + 0.001f * reflectedDir, reflectedDir);
			return true;
		}

	private: 
};

#endif
