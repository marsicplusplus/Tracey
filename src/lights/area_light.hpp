#ifndef __AREA_LIGHT_HPP__
#define __AREA_LIGHT_HPP__

#include "lights/light_object.hpp"

class AreaLight : public LightObject {

	public:
		AreaLight(glm::fvec3 pos, glm::fvec3 i, const std::shared_ptr<Hittable> &aLight);

		Ray getRay(const HitRecord &rec, float &tMax) const override;

		Color attenuate(Color color, const glm::fvec3 &p) override;

	protected:
		std::shared_ptr<Hittable> aLight;
		const float area;
};

#endif
