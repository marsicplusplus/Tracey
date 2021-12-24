#ifndef __MATERIAL_DIFFUSE_HPP__
#define __MATERIAL_DIFFUSE_HPP__

#include "materials/material.hpp"
#include "textures/solid_color.hpp"

class DiffuseMaterial : public Material {

	public: 
		DiffuseMaterial(std::string name, int albedoIdx) : Material{name} {
			albedo = albedoIdx;
		}

		inline Materials getType() const override { return Materials::DIFFUSE; }
};

#endif
