#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Sphere : public Hittable {
	public:
		Sphere(glm::dvec3 c, double r, MaterialPtr mat) : center{c}, radius{r}, radiusSquared{r*r}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			glm::dvec3 C = this->center - ray.getOrigin();
			double t = glm::dot(C, ray.getDirection());
			glm::dvec3 Q = C - t * ray.getDirection();
			float p2 = glm::dot(Q, Q); if(p2 > this->radiusSquared) return false;
			t -= sqrt(this->radiusSquared - p2);
			if((t < tMax) && (t > tMin) && (t > 0)){
				rec.t = t;
				rec.p = ray.at(t);
				rec.material = mat;
				glm::vec3 normal = (rec.p - center) / radius;
				rec.setFaceNormal(ray, normal);
				getUV(rec);
				return true;
			}
			return false;
		}

		inline void getUV(HitRecord &rec) const {
			double theta = std::acos(-rec.normal.y);
			double phi = std::atan2(-rec.normal.z, rec.normal.x) + PI;
			rec.u = static_cast<double>(phi)/static_cast<double>(2*PI);
			rec.v = static_cast<double>(theta)/static_cast<double>(PI);
		}

	private:
		glm::dvec3 center;
		double radius;
		double radiusSquared;
		MaterialPtr mat;
};

#endif
