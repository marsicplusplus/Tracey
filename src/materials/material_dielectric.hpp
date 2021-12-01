#ifndef __MATERIAL_DIELETRIC_HPP__
#define __MATERIAL_DIELETRIC_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"
#include "glm/gtx/norm.hpp"
#include <iostream>

class DielectricMaterial : public Material {

	public: 
		DielectricMaterial(Color color, double _refractIdx = 1.0, Color _absorption = Color(0, 0, 0)) : refractIdx{_refractIdx}, absorption{_absorption}{
			albedo = std::make_shared<SolidColor>(color);
		}

		DielectricMaterial(std::shared_ptr<Texture> t, double _refractIdx = 1.0, Color _absorption = Color(0, 0, 0)) : refractIdx{_refractIdx}, absorption{_absorption}{
			albedo = t;
		}

		inline Materials getType() const override { return Materials::DIELECTRIC; }

		inline bool reflect(const Ray& in, const HitRecord &r, Ray &reflectedRay, double &reflectance) const override {
			double n1 = (r.frontFace) ? 1.0 : refractIdx;
			double n2 = (r.frontFace) ? refractIdx : 1.0;
			double cosThetai = glm::dot(-in.getDirection(), r.normal);
			double ratio = n1/n2;
			double k = 1 - (ratio * ratio) * (1 - (cosThetai * cosThetai));
			if (k < 0){
				/* TIR */
				reflectance = 1.0;
			} else {
				/* Fresnel please help me */
				double sinThetai = sqrt(1 - (cosThetai * cosThetai));
				double cosThetat = sqrt(1.0 - ((ratio * sinThetai) * (ratio * sinThetai)));
				double rs = ((n1 * cosThetai - n2 * cosThetat) / (n1 * cosThetai + n2 * cosThetat));
				double rp = ((n1 * cosThetat - n2 * cosThetai) / (n1 * cosThetat + n2 * cosThetai));
				reflectance = (rs*rs+rp*rp)/2.0;
			}
			auto reflectDir = glm::reflect(in.getDirection(), r.normal);
			reflectedRay = Ray(r.p + 0.001 * reflectDir, reflectDir);
			return true;
		}

		inline virtual bool refract(const Ray& in, const HitRecord &r, Ray &refractedRay, double &refractance) const override {
			double cosi = glm::dot(-in.getDirection(), r.normal);
			double n1 = r.frontFace ? 1.0 : refractIdx;
			double n2 = r.frontFace ? refractIdx : 1.0;
			double ratio = n1/n2;
			double k = 1.0 - ratio * ratio * (1.0 - (cosi * cosi));
			if(k < 0){
				refractance = 0;
				return false;
			} else {
				glm::dvec3 refractedDir = ratio * in.getDirection() + (ratio * cosi - sqrt(k)) * r.normal;
				refractedRay = Ray(r.p + 0.001 * refractedDir, refractedDir);
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
		double refractIdx;
		Color absorption;
};

#endif
