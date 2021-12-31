#ifndef __LIGHT_OBJECT_HPP__
#define __LIGHT_OBJECT_HPP__

#include "defs.hpp"
#include "hittables/hittable.hpp"
#include <memory>

enum class LightType : int {
	LIGHT_DIRECTIONAL			= 1 << 0,
	LIGHT_POINT					= 1 << 1,
	LIGHT_SPOT					= 1 << 2,
	LIGHT_AMBIENT				= 1 << 3,
};

class LightObject {

	public:
		LightObject(int type, glm::fvec3 i) : type{type}, intensity{i} {}
		virtual ~LightObject() {};

		virtual Ray getRay(const HitRecord &rec, float &tMax) const = 0;

		virtual Color attenuate(Color color, const glm::fvec3& p) { return color; };

		virtual inline Color getLight(const HitRecord &rec, Ray& ray) const {
			Color illumination(0.0f);
			float nd = glm::dot(rec.normal, ray.getDirection());
			if(nd > 0.0f){
				illumination += this->intensity * nd;
			}
			return illumination;
		};

		const int type;
		const glm::fvec3 intensity;

	protected:
		Transform transform;
		
};

#endif
