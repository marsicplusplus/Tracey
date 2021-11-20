#ifndef __LAMBERTIAN_HPP__
#define __LAMBERTIAN_HPP__

#include "materials/material.hpp"
#include "glm/gtc/random.hpp"

class Lambertian : public Material {
	public:
		Lambertian(const Color& a) : albedo(a) {}
		virtual bool scatter(const Ray& in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override {
			auto scatterDir = rec.normal + glm::ballRand<double>(1);
			if(fabs(scatterDir.x) < 1e-8 && fabs(scatterDir.y) < 1e-8 && fabs(scatterDir.z) < 1e-8)
				scatterDir = rec.normal;
			scattered = Ray(rec.p, scatterDir);
			attenuation = albedo;
			return true;
		}

	private:
		Color albedo;
};

#endif
