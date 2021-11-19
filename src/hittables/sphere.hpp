#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Sphere : public Hittable {
	public:
		Sphere(glm::dvec3 c, double r) : center{c}, radius{r} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			glm::dvec3 oc = ray.getOrigin() - center;
			auto a = glm::length2(ray.getDirection());
			auto halfB = glm::dot(oc, ray.getDirection());
			auto c = glm::length2(oc) - radius * radius;
			auto discriminant = halfB*halfB - a*c;
			if (discriminant < 0) {
				return false;
			} else {
				double t0 = (-halfB - sqrt(discriminant) ) / (a);
				double t1 = (-halfB + sqrt(discriminant) ) / (a);
				double t = t0 < t1? t0 : t1;
				if(t < tMin || t > tMax) return false;
				rec.t = t;
				rec.p = ray.at(t);
				//glm::vec3 normal = glm::normalize(rec.p - center);
				//rec.setFaceNormal(ray, glm::normalize(normal));
				return true;
			}
		}

	private:
		glm::dvec3 center;
		double radius;
};

#endif
