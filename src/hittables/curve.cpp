#include "hittables/curve.hpp"

Curve::Curve(float uMin, float uMax, bool isClosed, int mat, const std::shared_ptr<CurveCommon>& common) :
	uMin(uMin),
	uMax(uMax),
	isClosed(isClosed), 
	mat(mat),
    common(common) {


    glm::fvec3 localCPts[4];
    localCPts[0] = BlossomBezier(common->curvePoints, uMin, uMin, uMin);
    localCPts[1] = BlossomBezier(common->curvePoints, uMin, uMin, uMax);
    localCPts[2] = BlossomBezier(common->curvePoints, uMin, uMax, uMax);
    localCPts[3] = BlossomBezier(common->curvePoints, uMax, uMax, uMax);

    for (int i = 0; i < 4; i++) {
        localBBox.minX = min(localBBox.minX, localCPts[i].x);
        localBBox.minY = min(localBBox.minY, localCPts[i].y);
        localBBox.minZ = min(localBBox.minZ, localCPts[i].z);
        localBBox.maxX = max(localBBox.maxX, localCPts[i].x);
        localBBox.maxY = max(localBBox.maxY, localCPts[i].y);
        localBBox.maxZ = max(localBBox.maxZ, localCPts[i].z);
    }

    float localWidths[2] = { 
        lerp(common->width[0], common->width[1], uMin),
        lerp(common->width[0], common->width[1], uMax)
    };

    float expandWidth = std::max(localWidths[0], localWidths[1]) * 0.5f;

    expandBBox(glm::fvec3(expandWidth, expandWidth, expandWidth));
}

bool Curve::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {

	//auto transformInv = transform.getInverse();
	//auto transposeInv = transform.getTransposeInverse();
	//auto transformMat = transform.getMatrix();
	//const auto transformedRay = ray.transformRay(transformInv);

	return false;
}

glm::fvec3 Curve::BlossomBezier(const glm::fvec3 cPts[4], float u0, float u1, float u2) {
    glm::fvec3 a[3] = { 
        lerp(cPts[0], cPts[1], u0),
        lerp(cPts[1], cPts[2], u0),
        lerp(cPts[2], cPts[3], u0)
    };

    glm::fvec3 b[2] = {
        lerp(a[0], a[1], u1),
        lerp(a[1], a[2], u1) 
    };

    return lerp(b[0], b[1], u2);
}