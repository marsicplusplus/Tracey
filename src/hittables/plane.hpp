#ifndef __PLANE_HPP__
#define __PLANE_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"

class ZXPlane : public Hittable {
	public:
		ZXPlane(double y, glm::dvec4 size, MaterialPtr mat) : pos{y}, size{size}, mat{mat} {}
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
			getUV(ray,rec);
			return true;
		}

		inline void getUV(const Ray &ray, HitRecord &rec) const override {
			double x = ray.getOrigin().x + rec.t * ray.getDirection().x;
			double z = ray.getOrigin().z + rec.t * ray.getDirection().z;

			rec.u = (x - size.x)/static_cast<double>(size.y - size.x);
			rec.u = (z - size.z)/static_cast<double>(size.z - size.w);
		}

	private:
		double pos;
		glm::dvec4 size;
		MaterialPtr mat;
};

#endif
