#ifndef __POINT_LIGHT_HPP__
#define __POINT_LIGHT_HPP__

#include "lights/light_object.hpp"

class PointLight : public LightObject {

	public:
		PointLight(glm::fvec3 pos, glm::fvec3 i);

		Ray getRay(const HitRecord &rec, float &tMax) const override;

		Color attenuate(Color color, const glm::fvec3 &p) override;
};

#endif
