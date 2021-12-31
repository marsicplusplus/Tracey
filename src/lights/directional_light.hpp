#ifndef __DIRECTIONAL_LIGHT_HPP__
#define __DIRECTIONAL_LIGHT_HPP__

#include "lights/light_object.hpp"

class DirectionalLight : public LightObject {

	public:
		DirectionalLight(glm::fvec3 dir, Color i);

		Ray getRay(const HitRecord &rec, float &tMax) const override;

	private:
		glm::fvec3 direction;
};

#endif
