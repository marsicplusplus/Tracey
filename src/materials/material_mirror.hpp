#ifndef __MATERIAL_MIRROR_HPP__
#define __MATERIAL_MIRROR_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class MirrorMaterial : public Material {

	public: 
		MirrorMaterial(std::string name, int albedoIdx, float _reflective = 1.0f) : Material{name}, reflectionIdx{_reflective} {
			albedo = albedoIdx;
		}

		inline Materials getType() const override { return Materials::MIRROR; }

		inline bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay) const override {
			auto reflectedDir = glm::reflect(in.getDirection(), r.normal);
			reflectedRay = Ray(r.p + 0.001f * reflectedDir, reflectedDir);
			return true;
		}

		inline float getReflection() const { return reflectionIdx; }

	private: 
		float reflectionIdx;
};

#endif
