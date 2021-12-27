#ifndef __MATERIAL_EMISSIVE_HPP__
#define __MATERIAL_EMISSIVE_HPP__

#include "materials/material.hpp"
#include <iostream>

class EmissiveMaterial : public Material {

	public: 
		EmissiveMaterial(std::string name, int albedoIdx, float _intensity = 1.0f) : Material{name} {
			albedo = albedoIdx;
			intensity = _intensity;
		}
		inline Materials getType() const override { return Materials::EMISSIVE; }
	private: 
};

#endif
