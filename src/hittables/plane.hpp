#ifndef __PLANE_HPP__
#define __PLANE_HPP__

#include <iostream>
#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class Plane : public Hittable {
	public:
		Plane(glm::dvec3 pos, glm::dvec3 norm, MaterialPtr mat) : pos{pos}, norm{norm}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			double d = glm::dot(this->norm, ray.getDirection());
			if(std::abs(d) > 1e-6){
				glm::dvec3 p = pos - ray.getOrigin();
				double t = glm::dot(p, norm) / static_cast<double>(d);
				if(t >= 1e-6 && t > tMin && t < tMax) {
					rec.t = t;
					rec.setFaceNormal(ray, norm);
					rec.material = mat;
					rec.p = ray.at(t);
					getUV(rec);
					return true;
				}
			}
			return false;
		}

		inline void getUV(HitRecord &rec) const {
			glm::dvec3 uAxis = glm::dvec3(1,0,0);
			glm::dvec3 vAxis = glm::dvec3(0,0,-1);

			rec.u = glm::dot(rec.p, uAxis);
			rec.v = glm::dot(rec.p, vAxis);
		}

	private:
		glm::dvec3 pos;
		glm::dvec3 norm;
		MaterialPtr mat;
};

class ZXRect : public Hittable {
	public:
		ZXRect(double y, glm::dvec4 size, MaterialPtr mat) : pos{y}, size{size}, mat{mat} {}
		inline bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const override {
			double t = (pos - ray.getOrigin().y)/static_cast<double>(ray.getDirection().y);
			if(t < tMin || t > tMax) return false;
			double x = ray.getOrigin().x + t * ray.getDirection().x;
			double z = ray.getOrigin().z + t * ray.getDirection().z;
			if(x < size.x || x > size.y || z < size.z || z > size.w) return false;
			rec.t = t;
			rec.setFaceNormal(ray, glm::dvec3(0.0, 1.0, 0.0));
			rec.material = mat;
			rec.p = ray.at(t);
			getUV(rec);
			return true;
		}

		inline void getUV(HitRecord &rec) const {
			rec.u = (rec.p.x - size.x)/static_cast<double>(size.y - size.x);
			rec.v = (rec.p.z - size.z)/static_cast<double>(size.z - size.w);
		}

	private:
		double pos;
		glm::dvec4 size;
		MaterialPtr mat;
};

#endif
