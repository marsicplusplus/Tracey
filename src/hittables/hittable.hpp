#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include <memory>

class Hittable {
	public:
		Hittable() : transform{glm::dmat4x4(1.0)}, transformInv{glm::dmat4x4(1.0)} {}
		virtual bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const = 0;

		inline void translate(glm::dvec3 t){
			transform = glm::translate(transform, t);
			transformInv = glm::translate(transformInv, -t);
		}
		inline void scale(glm::dvec3 s){
			transform = glm::scale(transform, s);
			transformInv = glm::scale(transformInv, 1.0/s);
		}
		inline void scale(float s){
			transform = glm::scale(transform, glm::dvec3(s,s,s));
			transformInv = glm::scale(transformInv, glm::dvec3(1.0/s,1.0/s,1.0/s));
		}
		inline void rotate(double t, glm::dvec3 a){
			transform = glm::rotate(transform, t, a);
			transformInv = glm::rotate(transformInv, -t, a);
		}

	protected:
		glm::dvec3 position;
		glm::dmat4x4 transformInv;
		glm::dmat4x4 transform;

};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
