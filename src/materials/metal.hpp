#ifndef __METAL_HPP__
#define __METAL_HPP__

#include "textures/texture.hpp"
#include "textures/solid_color.hpp"
#include "materials/material.hpp"
#include "glm/gtc/random.hpp"

class Metal : public Material {
	public:
		Metal(const Color& a, double fuzz = 0.0) : albedo(std::make_shared<SolidColor>(a)), fuzz{fuzz} {}
		Metal(std::shared_ptr<Texture> a, double fuzz = 0.0) : albedo(a), fuzz{fuzz} {}
		virtual bool scatter(const Ray& in, const HitRecord& rec, Color& attenuation, Ray& scattered) const override {
			glm::dvec3 reflected = glm::reflect(glm::normalize(in.getDirection()), rec.normal);
			scattered = Ray(rec.p, reflected+fuzz*glm::ballRand(1.0));
			attenuation = albedo->color(rec.u, rec.v, rec.p);
			return (glm::dot(scattered.getDirection(), rec.normal) > 0);
		}

	private:
		std::shared_ptr<Texture> albedo;
		double fuzz;
};

#endif
