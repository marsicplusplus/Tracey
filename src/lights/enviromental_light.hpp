#ifndef __ENVIROMENTAL_LIGHT_HPP__
#define __ENVIROMENTAL_LIGHT_HPP__

#include "lights/light_object.hpp"

class EnviromentalLight : public LightObject {

	public:
		EnviromentalLight(glm::fvec3 pos, glm::fvec3 i, const std::shared_ptr<Texture> &texture);

		Ray getRay(const HitRecord &rec, float &tMax) const override;

		Color attenuate(Color color, const glm::fvec3 &p) override;

	protected:
		std::shared_ptr<Texture> texture;
		const float area;
};

#endif
