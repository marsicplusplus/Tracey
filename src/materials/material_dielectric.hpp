#ifndef __MATERIAL_DIELETRIC_HPP__
#define __MATERIAL_DIELETRIC_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"
#include "glm/gtx/norm.hpp"
#include <iostream>

class DielectricMaterial : public Material {

	public: 
		DielectricMaterial(Color color, float _ior = 1.0f, Color _absorption = Color(0, 0, 0)) : ior{_ior}, absorption{_absorption}{
			albedo = std::make_shared<SolidColor>(color);
		}

		DielectricMaterial(std::shared_ptr<Texture> t, float _ior = 1.0f, Color _absorption = Color(0, 0, 0)) : ior{_ior}, absorption{_absorption}{
			albedo = t;
		}

		inline Materials getType() const override { return Materials::DIELECTRIC; }

		inline bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, float &reflectance) const override {
			float n1 = (r.frontFace) ? 1.0f : ior;
			float n2 = (r.frontFace) ? ior : 1.0f;
			float cosThetai = glm::dot(-in.getDirection(), r.normal);
			float ratio = n1/n2;
			float k = 1 - (ratio * ratio) * (1.0f - (cosThetai * cosThetai));
			if (k < 0.0f){
				/* TIR */
				reflectance = 1.0f;
			} else {
				/* Fresnel please help me */
				float sinThetai = sqrt(1.0f - (cosThetai * cosThetai));
				float cosThetat = sqrt(1.0f - ((ratio * sinThetai) * (ratio * sinThetai)));
				float rs = ((n1 * cosThetai - n2 * cosThetat) / (n1 * cosThetai + n2 * cosThetat));
				float rp = ((n1 * cosThetat - n2 * cosThetai) / (n1 * cosThetat + n2 * cosThetai));
				reflectance = (rs*rs+rp*rp)/2.0f;
			}
			auto reflectDir = glm::reflect(in.getDirection(), r.normal);
			reflectedRay = Ray(r.p + 0.001f * reflectDir, reflectDir);
			return true;
		}

		inline virtual bool refract(const Ray& in, const HitRecord &r, Ray &refractedRay, float &refractance) const override {
			float cosi = glm::dot(-in.getDirection(), r.normal);
			float n1 = r.frontFace ? 1.0f : ior;
			float n2 = r.frontFace ? ior : 1.0f;
			float ratio = n1/n2;
			float k = 1.0f - ratio * ratio * (1.0f - (cosi * cosi));
			if(k < 0.0f){
				refractance = 0.0f;
				return false;
			} else {
				glm::fvec3 refractedDir = ratio * in.getDirection() + (ratio * cosi - sqrtf(k)) * r.normal;
				refractedRay = Ray(r.p + 0.001f * refractedDir, refractedDir);
				return true;
			}
		}

		inline void absorb(const Ray& in, const HitRecord& r, Color& attenuation) const override {
			if (!r.frontFace) {
				const auto distance = glm::distance(in.getOrigin(), r.p);
				attenuation.r *= exp(-absorption.r * distance);
				attenuation.g *= exp(-absorption.g * distance);
				attenuation.b *= exp(-absorption.b * distance);
			}
		}

	private: 
		float ior;
		Color absorption;
};

#endif
