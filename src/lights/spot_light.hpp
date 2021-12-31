#ifndef __SPOT_LIGHT_HPP__
#define __SPOT_LIGHT_HPP__

#include "lights/light_object.hpp"

class SpotLight : public LightObject {

public:
	SpotLight(glm::fvec3 pos, glm::fvec3 dir, float cutoff, glm::fvec3 i);

	Ray getRay(const HitRecord& rec, float& tMax) const override;

	Color attenuate(Color color, const glm::fvec3& p) override;

private:
	glm::fvec3 direction;
	float cutoffAngle;
};

#endif
