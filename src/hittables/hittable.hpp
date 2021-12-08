#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include <memory>

class Hittable {
	public:
		Hittable() : transform{glm::fmat4x4(1.0)}, transformInv{glm::fmat4x4(1.0)} {}
		virtual bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const = 0;

		inline void translate(glm::fvec3 t){
			transform = glm::translate(transform, t);
			transformInv = glm::inverse(transform);
		}
		inline void scale(glm::fvec3 s){
			transform = glm::scale(transform, s);
			transformInv = glm::inverse(transform);
		}
		inline void scale(float s){
			transform = glm::scale(transform, glm::fvec3(s,s,s));
			transformInv = glm::inverse(transform);
		}
		inline void rotate(float t, glm::fvec3 a){
			transform = glm::rotate(transform, t, a);
			transformInv = glm::inverse(transform);
		}

	protected:
		glm::fvec3 position;
		glm::fmat4x4 transformInv;
		glm::fmat4x4 transform;

};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
