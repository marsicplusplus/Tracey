#ifndef __AMBIENT_LIGHT_HPP__
#define __AMBIENT_LIGHT_HPP__

#include "lights/light_object.hpp"

class AmbientLight : public LightObject {

	public:
		AmbientLight(Color i);

		Ray getRay(const HitRecord &rec, float &tMax) const override;

		Color getLight(const HitRecord &rec, Ray& ray) const override;
};

typedef std::unique_ptr<LightObject> LightObjectPtr;

#endif
