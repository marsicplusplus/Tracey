#ifndef __CHECKERED_HPP__
#define __CHECKERED_HPP__

#include "materials/material.hpp"
#include "glm/gtc/random.hpp"

/* TODO: This is an hack, will be replaced by texture */
class Checkered : public Material {
	public:
		Checkered(Color c1, Color c2) : c1{c1}, c2{c2} {}
		virtual bool scatter(const Ray& in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override {
			auto scatterDir = rec.normal + glm::ballRand<double>(1);
			if(fabs(scatterDir.x) < 1e-8 && fabs(scatterDir.y) < 1e-8 && fabs(scatterDir.z) < 1e-8)
				scatterDir = rec.normal;
			scattered = Ray(rec.p, scatterDir);
			int a = floor(in.u * 10.0) + floor(in.v * 10.0);
			if(a % 2 == 0)
				attenuation = c1;
			else
				attenuation = c2;
			return true;
		}

	private:
		Color c1;
		Color c2;
};

#endif
