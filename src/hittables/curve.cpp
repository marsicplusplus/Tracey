#include "hittables/curve.hpp"

Curve::Curve(std::vector<glm::fvec3> cPts, bool isClosed, int mat) :
	controlPoints(cPts), 
	isClosed(isClosed), 
	mat(mat) {

    uMin = 0;
    uMax = 1;

    float startWidth = 0.001167;
    float endWidth = 0.000167;
    for (const auto& cp : cPts) {
        localBBox.minX = min(localBBox.minX, cp.x);
        localBBox.minY = min(localBBox.minY, cp.y);
        localBBox.minZ = min(localBBox.minZ, cp.z);
        localBBox.maxX = max(localBBox.maxX, cp.x);
        localBBox.maxY = max(localBBox.maxY, cp.y);
        localBBox.maxZ = max(localBBox.maxZ, cp.z);
    }

    float width[2] = { lerp(startWidth, endWidth, uMin),
                       lerp(startWidth, endWidth, uMax) };

    float expandWidth = std::max(width[0], width[1]) * 0.5f;
    expandBBox(glm::fvec3(expandWidth, expandWidth, expandWidth));
}

bool Curve::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	return false;
}
