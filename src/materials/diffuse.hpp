#ifndef __DIFFUSE_HPP__
#define __DIFFUSE_HPP__

#include "textures/solid_color.hpp"
#include "materials/material.hpp"
#include "glm/gtc/random.hpp"

class Diffuse : public Material {
	public:
		Diffuse(const Color& a) : albedo(std::make_shared<SolidColor>(a)) {}
		Diffuse(std::shared_ptr<Texture> a) : albedo(a) {}
		virtual bool scatter(const Ray& in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override {
			auto scatterDir = rec.normal + glm::normalize(glm::ballRand<double>(1));
			if(fabs(scatterDir.x) < 1e-8 && fabs(scatterDir.y) < 1e-8 && fabs(scatterDir.z) < 1e-8)
				scatterDir = rec.normal;
			scattered = Ray(rec.p, scatterDir);
			attenuation = albedo->color(rec.u, rec.v, rec.p);
			return true;
		}

	private:
		std::shared_ptr<Texture> albedo;
};

#endif
