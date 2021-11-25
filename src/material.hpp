#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "textures/texture.hpp"
#include "textures/solid_color.hpp"

#include <memory>
#include <random>

struct HitRecord;
typedef glm::dvec3 Color;

class Material {
	public:
		Material(std::shared_ptr<Texture> a, double _reflective = 0.0, double _specular = 0.0, double _refraction = 0.0) : albedo{a}, reflect{_reflective}, specular{_specular}, refraction{_refraction} {};
		Material(const Color& a, double _reflective = 0.0, double _specular = 0.0, double _refraction = 0.0) : albedo{std::make_shared<SolidColor>(a)}, reflect{_reflective}, specular{_specular}, refraction{_refraction} {}

		virtual Color getAlbedo(HitRecord &rec) const;
		virtual double getSpecular() const;
		virtual double getReflective() const;
		virtual double getRefractionIndex() const;

	protected:
		std::shared_ptr<Texture> albedo;
		double specular;
		double reflect;
		double refraction;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
