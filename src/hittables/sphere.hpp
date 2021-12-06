#ifndef __SPHERE_HPP__
#define __SPHERE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Sphere : public Hittable {
	public:
		Sphere(double r, MaterialPtr mat) : Hittable(), radius{r}, radiusSquared{r*r}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			const auto transformedRay = ray.transformRay(transformInv);
			const auto transformedRayDir = transformedRay.getDirection();
			const auto transformedOrigin = transformedRay.getOrigin();

			if(mat->getType() != Materials::DIELECTRIC){ 
				glm::dvec3 C = glm::dvec3(0.0) - transformedOrigin;
				double t = glm::dot(C, transformedRayDir);
				glm::dvec3 Q = C - t * transformedRayDir;
				float p2 = glm::dot(Q, Q); if(p2 > this->radiusSquared) return false;
				t -= sqrt(this->radiusSquared - p2);
				if((t < tMax) && (t > tMin) && (t > 0)){
					rec.t = t;
					rec.p = transformedRay.at(t);
					rec.material = mat;
					glm::dvec3 normal = (rec.p) / radius;
					auto transformedNormal = transform * glm::dvec4(normal, 0);
					rec.setFaceNormal(ray, transformedNormal);
					getUV(rec);
					return true;
				}
				return false;
			} else {
			/* Inefficent, but needed */
				glm::dvec3 oc = transformedOrigin;
				auto a = glm::dot(transformedRayDir, transformedRayDir);
				auto b = 2.0 * glm::dot(oc, transformedRayDir);
				auto c = glm::dot(oc, oc) - radius*radius;
				auto discriminant = b*b - 4*a*c;
				if(discriminant >= 0){
					double sq = sqrt(discriminant);
					double t0 = (-b - sq ) / (2.0*a);
					double t1 = (-b + sq ) / (2.0*a);
					double cT;
					if(t0 < t1 && t0 > 0) cT = t0;
					else if(t1 > 0) cT = t1;
					else return false;
					if(cT < tMax && cT > tMin){
						rec.t = cT;
						rec.p = transformedRay.at(cT);
						rec.material = mat;
						glm::dvec3 normal = (rec.p) / radius;
						auto transformedNormal = transform * glm::dvec4(normal, 0);
						rec.setFaceNormal(ray, transformedNormal);
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
		double radius;
		double radiusSquared;
		MaterialPtr mat;
};

#endif
