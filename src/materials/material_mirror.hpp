#ifndef __MATERIAL_MIRROR_HPP__
#define __MATERIAL_MIRROR_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class MirrorMaterial : public Material {
	public: 
		MirrorMaterial(Color color, double _reflective = 1.0) : albedo(std::make_shared<SolidColor>(color)), reflectionIdx{_reflective} {}
		MirrorMaterial(std::shared_ptr<Texture> t, double _reflective = 1.0) : albedo(t) , reflectionIdx{_reflective} {}
		inline bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double &s) const override {
			s = reflectionIdx;
			attenuation = albedo->color(r.u, r.v, r.p);
			auto reflectedDir = glm::reflect(in.getDirection(), r.normal);
			scattered = Ray(r.p + 0.001 * reflectedDir, reflectedDir);
			return true;
		}

	private: 
		std::shared_ptr<Texture> albedo;
		double reflectionIdx;
};

#endif
