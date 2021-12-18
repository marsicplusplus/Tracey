#ifndef __INSTANCE_HPP__
#define __INSTANCE_HPP__

#include "hittables/mesh.hpp"

class Instance : public Hittable {
	public:
		Instance(std::shared_ptr<Hittable> hit) : Hittable(hit->getLocalAABB()), hittable{hit} {};

		inline bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override {
			const auto transformedRay = ray.transformRay(transformInv);
			HitRecord tmp;
			if(hittable->hit(transformedRay, tMin, tMax, tmp)){
				rec = tmp;
				rec.setFaceNormal(ray, transposeInv * glm::fvec4(tmp.normal, 0.0));
				rec.p = transform * glm::fvec4(tmp.p, 1.0);
				return true;
			}
			return false;
		}

	private:
		std::shared_ptr<Hittable> hittable;
};

#endif
