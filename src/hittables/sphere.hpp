#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Sphere : public Hittable {
	public:
		Sphere(glm::fvec3 c, float r, MaterialPtr mat) : center{c}, radius{r}, radiusSquared{r*r}, mat{mat} {}
		inline bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const override {
			if(mat->getType() != Materials::DIELECTRIC){ 
				glm::fvec3 C = this->center - ray.getOrigin();
				float t = glm::dot(C, ray.getDirection());
				glm::fvec3 Q = C - t * ray.getDirection();
				float p2 = glm::dot(Q, Q); if(p2 > this->radiusSquared) return false;
				t -= sqrt(this->radiusSquared - p2);
				if((t < tMax) && (t > tMin) && (t > 0)){
					rec.t = t;
					rec.p = ray.at(t);
					rec.material = mat;
					glm::fvec3 normal = (rec.p - center) / radius;
					rec.setFaceNormal(ray, normal);
					getUV(rec);
					return true;
				}
				return false;
			} else {
			/* Inefficent, but needed */
				glm::fvec3 oc = ray.getOrigin() - center;
				auto a = glm::dot(ray.getDirection(), ray.getDirection());
				auto b = 2.0 * glm::dot(oc, ray.getDirection());
				auto c = glm::dot(oc, oc) - radius*radius;
				auto discriminant = b*b - 4*a*c;
				if(discriminant >= 0){
					float sq = sqrt(discriminant);
					float t0 = (-b - sq ) / (2.0*a);
					float t1 = (-b + sq ) / (2.0*a);
					float cT;
					if(t0 < t1 && t0 > 0) cT = t0;
					else if(t1 > 0) cT = t1;
					else return false;
					if(cT < tMax && cT > tMin){
						rec.t = cT;
						rec.p = ray.at(cT);
						rec.material = mat;
						glm::fvec3 normal = (rec.p - center) / radius;
						rec.setFaceNormal(ray, normal);
						getUV(rec);
						return true;
					}
				}
				return false;
			}
		}

		inline void getUV(HitRecord &rec) const {
			float theta = std::acos(-rec.normal.y);
			float phi = std::atan2(-rec.normal.z, rec.normal.x) + PI;
			rec.u = static_cast<float>(phi)/static_cast<float>(2*PI);
			rec.v = static_cast<float>(theta)/static_cast<float>(PI);
		}

	private:
		glm::fvec3 center;
		float radius;
		float radiusSquared;
		MaterialPtr mat;
};

#endif
