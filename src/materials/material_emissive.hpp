#ifndef __MATERIAL_EMISSIVE_HPP__
#define __MATERIAL_EMISSIVE_HPP__

#include "materials/material.hpp"
#include <iostream>

class EmissiveMaterial : public Material {

	public: 
		EmissiveMaterial(std::string name, int albedoIdx, float r = 1.0f, float g = 1.0f, float b = 1.0f) : Material{name} {
			EmissiveMaterial(name, albedoIdx, {r, g, b});
		}
		EmissiveMaterial(std::string name, int albedoIdx, glm::fvec3 _intensity = glm::fvec3{1.0f}) : Material{name} {
			albedo = albedoIdx;
			intensity = _intensity;
		}
		inline Materials getType() const override { return Materials::EMISSIVE; }

		inline glm::fvec3 getIntensity() const { return intensity; }
	private: 
		glm::fvec3 intensity;
};

#endif
