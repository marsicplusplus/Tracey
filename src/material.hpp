#ifndef __MATERIAL_HPP__
#define __MATERIAL_HPP__

#include "textures/texture.hpp"
#include "textures/solid_color.hpp"

#include <memory>
#include <random>

enum class Materials{
	Diffuse,
	Mirror,
};

struct HitRecord;
typedef glm::dvec3 Color;

class Material {
	public:
		Material(Materials type, std::shared_ptr<Texture> a) : materialType{type}, albedo{a} {};
		Material(Materials type, const Color& a) : materialType{type}, albedo{std::make_shared<SolidColor>(a)} {}

		virtual Color getAlbedo(HitRecord &rec) const;
		virtual Materials getType() const;

	protected:
		std::shared_ptr<Texture> albedo;
		Materials materialType;
};

typedef std::shared_ptr<Material> MaterialPtr;

#endif
