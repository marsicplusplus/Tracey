#include "hittables/curve.hpp"

Curve::Curve(std::vector<glm::fvec3> cPts, bool isClosed, int mat) : 
	controlPoints(cPts), 
	isClosed(isClosed), 
	mat(mat) {}

bool Curve::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	return false;
}
