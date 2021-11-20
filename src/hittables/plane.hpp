#ifndef __PLANE_HPP__
#define __PLANE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Plane : public Hittable {
	public:
		Plane(glm::dvec3 pos, glm::dvec3 norm, MaterialPtr mat) : position{pos}, normal{glm::normalize(norm)}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			float denom = glm::dot(this->normal, ray.getDirection()); 
			if (denom > 1e-6) { 
				glm::dvec3 p0l0 = position - ray.getOrigin(); 
				rec.t = glm::dot(p0l0, this->normal) / denom; 
				rec.p = ray.at(rec.t);
				rec.setFaceNormal(ray, this->normal);
				rec.material = mat;
				return (rec.t >= 0 && rec.t < tMax && rec.t > tMin); 
			} 
			return false;
		}

	private:
		glm::dvec3 position;
		glm::dvec3 normal;
		MaterialPtr mat;
};

#endif
