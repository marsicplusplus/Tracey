#ifndef __MATERIAL_MIRROR_HPP__
#define __MATERIAL_MIRROR_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class MirrorMaterial : public Material {

	public: 
		MirrorMaterial(Color color, float _reflective = 1.0f) :  reflectionIdx{_reflective} {
			albedo = std::make_shared<SolidColor>(color);
		}

		MirrorMaterial(std::shared_ptr<Texture> t, float _reflective = 1.0f) : reflectionIdx{_reflective} {
			albedo = t;
		}

		inline Materials getType() const override { return Materials::MIRROR; }

		inline bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, float &reflectance) const override {
			reflectance = reflectionIdx;
			auto reflectedDir = glm::reflect(in.getDirection(), r.normal);
			reflectedRay = Ray(r.p + 0.001f * reflectedDir, reflectedDir);
			return true;
		}

	private: 
		float reflectionIdx;
};

#endif
