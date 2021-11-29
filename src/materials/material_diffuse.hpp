#ifndef __MATERIAL_DIFFUSE_HPP__
#define __MATERIAL_DIFFUSE_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class DiffuseMaterial : public Material {
	public: 
		DiffuseMaterial(Color color) : albedo(std::make_shared<SolidColor>(color)) {}
		DiffuseMaterial(std::shared_ptr<Texture> t) : albedo(t) {}
		DiffuseMaterial() : albedo(std::make_shared<SolidColor>(0.5, 0.5, 0.5)) {}
		inline bool reflect(const Ray& in, const HitRecord &r, Color& attenuation, Ray &scattered, double& s) const override {
			s = 0.0;
			attenuation = albedo->color(r.u, r.v, r.p);
			return false;
		}

	private: 
		std::shared_ptr<Texture> albedo;
};

#endif
