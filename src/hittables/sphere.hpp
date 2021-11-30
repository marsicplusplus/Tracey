#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

#include <iostream>

class Sphere : public Hittable {
	public:
		Sphere(glm::dvec3 c, double r, MaterialPtr mat) : center{c}, radius{r}, radiusSquared{r*r}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			if(mat->getType() != Materials::DIELECTRIC){ 
				glm::dvec3 C = this->center - ray.getOrigin();
				double t = glm::dot(C, ray.getDirection());
				glm::dvec3 Q = C - t * ray.getDirection();
				float p2 = glm::dot(Q, Q); if(p2 > this->radiusSquared) return false;
				t -= sqrt(this->radiusSquared - p2);
				if((t < tMax) && (t > tMin) && (t > 0)){
					rec.t = t;
					rec.p = ray.at(t);
					rec.material = mat;
					glm::dvec3 normal = (rec.p - center) / radius;
					rec.setFaceNormal(ray, normal);
					if(!rec.frontFace){
						std::cout << "I'M IN" << std::endl;
					}
					getUV(rec);
					return true;
				}
				return false;
			} else {
			/* Inefficent, but needed */
				glm::dvec3 oc = ray.getOrigin() - center;
				auto a = glm::dot(ray.getDirection(), ray.getDirection());
				auto b = 2.0 * glm::dot(oc, ray.getDirection());
				auto c = glm::dot(oc, oc) - radius*radius;
				auto discriminant = b*b - 4*a*c;
				if(discriminant >= 0){
					double t0 = (-b - sqrt(discriminant) ) / (2.0*a);
					double t1 = (-b + sqrt(discriminant) ) / (2.0*a);
					double cT;
					if(t0 < t1 && t0 > 0) cT = t0;
					else if(t1 > 0) cT = t1;
					else return false;
					if(cT < tMax && cT > tMin){
						rec.t = cT;
						rec.p = ray.at(cT);
						rec.material = mat;
						glm::dvec3 normal = (rec.p - center) / radius;
						rec.setFaceNormal(ray, normal);
						getUV(rec);
						return true;
					}
				}
				return false;
			}
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
