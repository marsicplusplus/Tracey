#ifndef __MATERIAL_DIFFUSE_HPP__
#define __MATERIAL_DIFFUSE_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class DiffuseMaterial : public Material {

	public: 
		DiffuseMaterial(Color color, std::string name) : Material{name} {
			albedo = std::make_shared<SolidColor>(color);
		}

		DiffuseMaterial(std::shared_ptr<Texture> t, std::string name) : Material{name} {
			albedo = t;
		}

		DiffuseMaterial(std::string name) : Material{name} {
			albedo = std::make_shared<SolidColor>(0.5f, 0.5f, 0.5f);
		}

		inline Materials getType() const override { return Materials::DIFFUSE; }

};

#endif
