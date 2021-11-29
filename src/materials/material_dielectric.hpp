#ifndef __MATERIAL_DIELETRIC_HPP__
#define __MATERIAL_DIELETRIC_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"
#include "glm/gtx/norm.hpp"

class DielectricMaterial : public Material {
	public: 
		DielectricMaterial(Color color, double _idx = 1.0) : albedo(std::make_shared<SolidColor>(color)), idx{_idx} {}
		DielectricMaterial(std::shared_ptr<Texture> t, double _idx = 1.0) : albedo(t) , idx{_idx} {}
		inline bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const override {
			attenuation = albedo->color(r.u, r.v, r.p);
			double ratio = r.frontFace ? in.getCurrentRefraction() / static_cast<double>(idx) : idx / static_cast<double>(in.getCurrentRefraction());
			glm::dvec3 normDir = glm::normalize(in.getDirection());
			double cosTetha = std::min(dot(-normDir, r.normal), 1.0);
			double k = 1 - ratio*ratio * (1 - cosTetha * cosTetha);

			glm::dvec3 newDir;
			double newIdx = idx;
			if(k < 0.0){
				newDir = glm::reflect(normDir, r.normal);
				newIdx = in.getCurrentRefraction();
			} else {
				newDir = glm::refract(normDir, r.normal, ratio);
			}
			scattered = Ray(r.p + 0.0001 * newDir, newDir, newIdx);

			return true;
		}

	private: 
		std::shared_ptr<Texture> albedo;
		double idx;
};

#endif
