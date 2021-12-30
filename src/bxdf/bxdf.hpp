#ifndef __BXDF_HPP__
#define __BXDF_HPP__

#include "defs.hpp"

enum class BxDFType {
	BSDF_REFLECTION 		= 1 << 0,
	BSDF_SPECULAR			= 1 << 1,
	BSDF_GLOSSY				= 1 << 2,
	BSDF_TRANSMISSION		= 1 << 3,
	BSDF_DIFFUSE			= 1 << 4,
	ALL						= BSDF_GLOSSY | BSDF_DIFFUSE
								| BSDF_SPECULAR | BSDF_REFLECTION 
								| BSDF_TRANSMISSION,
};

class BxDF {
	public:
		BxDF(BxDFType _type) : type{_type} {}
		virtual Color f(const glm::fvec3 &wo, const glm::fvec3 &wi) const = 0;


		BxDFType type;
};

#endif
