#include "hittables/curve.hpp"

Curve::Curve(float uMin, float uMax, bool isClosed, int mat, const std::shared_ptr<CurveCommon>& common) :
	uMin(uMin),
	uMax(uMax),
	isClosed(isClosed), 
	mat(mat),
	common(common) {


		glm::fvec3 localCPts[4];
		getLocalControlPoints(localCPts);

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

		float expandWidth = max(localWidths[0], localWidths[1]) * 0.5f;

		expandBBox(localBBox, glm::fvec3(expandWidth));

		auto axisDir = localCPts[3] - localCPts[0]; /* Axis direction */
		auto oe = ((localCPts[3] + localCPts[0])/2.0f
				+ EvalBezier(localCPts, 0.5, nullptr)) / 2.0f; /* Cylinder passes through this point */
		float rMax = max(localWidths[0], localWidths[1]);  /* Radius of the cylinder */

		// We should subdivide the curve a predefinite number of times;
		glm::fvec3 cpSplit[7];
		SubdivideBezier(localCPts, cpSplit);
		float de = -INF;
		const int ctrlPtsNo = 7;
		for(int i = 0; i < ctrlPtsNo; ++i){
			auto dist = glm::length(glm::cross(cpSplit[i] - localCPts[0], axisDir)) / glm::length(axisDir);
			de = max(dist, de);
		}

		enclosingCylinder = Cylinder{
			axisDir,
			oe,
			rMax,
			de
		};
}


glm::fvec3 Curve::BlossomBezier(const glm::fvec3 cPts[4], float u0, float u1, float u2) const {
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

void Curve::getLocalControlPoints(glm::fvec3 *pts) const {
	pts[0] = BlossomBezier(common->curvePoints, uMin, uMin, uMin);
	pts[1] = BlossomBezier(common->curvePoints, uMin, uMin, uMax);
	pts[2] = BlossomBezier(common->curvePoints, uMin, uMax, uMax);
	pts[3] = BlossomBezier(common->curvePoints, uMax, uMax, uMax);
}

bool Curve::hitEnclosingCylinder(const Ray& ray) const {
	auto n = glm::cross(ray.getDirection(), enclosingCylinder.axis);
	auto tmp = glm::dot(ray.getOrigin() - enclosingCylinder.oe, n);
	auto dSquared = (tmp * tmp)/(glm::dot(n, n));

	if (sqrt(dSquared) > enclosingCylinder.radiusMax + enclosingCylinder.de) return false;

	return true;
}

bool Curve::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	return hitPhantom(ray, tMin, tMax, rec);
}

bool Curve::hitPhantom(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	// Early check for enclosing cylinder
	if (!hitEnclosingCylinder(ray)) return false;

	glm::fvec3 localCPts[4];
	localCPts[0] = BlossomBezier(common->curvePoints, uMin, uMin, uMin);
	localCPts[1] = BlossomBezier(common->curvePoints, uMin, uMin, uMax);
	localCPts[2] = BlossomBezier(common->curvePoints, uMin, uMax, uMax);
	localCPts[3] = BlossomBezier(common->curvePoints, uMax, uMax, uMax);

	auto upVec = glm::normalize(glm::cross(localCPts[3], ray.getDirection()));
	const auto objectToRay = glm::lookAt(ray.getOrigin(), ray.getOrigin() - ray.getDirection(), upVec);

	glm::fvec3 transformedPoints[4] = { objectToRay * glm::vec4(localCPts[0], 1), objectToRay * glm::vec4(localCPts[1], 1),
					  objectToRay * glm::vec4(localCPts[2], 1), objectToRay * glm::vec4(localCPts[3], 1) };

	float tStart = glm::dot(transformedPoints[3] - transformedPoints[0], ray.getDirection()) > 0.0f ? 0.0f : 1.0f;

	bool hit = false;
	float localWidths[2] = {
		lerp(common->width[0], common->width[1], uMin),
		lerp(common->width[0], common->width[1], uMax)
	};

	for (int side = 0; side < 2; ++side) {
		float t = tStart;
		float tOld = 0.0f;

		RayConeIntersection inters;

		float dt1 = 0.0f;
		float dt2 = 0.0f;

		/* Max number of iterations in the paper for a model was 36 */
		for (int iter = 0; iter < 40; ++iter) {
			inters.c0 = EvalBezier(transformedPoints, t, nullptr);
			inters.cd = getTangent(transformedPoints, t);
			bool realHit = inters.intersect(std::max(localWidths[0], localWidths[1]), 0.0f);

			if (realHit && fabsf(inters.dt) < 5e-5f) { /* Stops at 5e-5 as in the paper */

				auto t = (inters.s + inters.c0.z) / glm::length(ray.getDirection());
				if (t < tMin || t > tMax) {
					break;
				}

				// Fill in hit struct and break;
				rec.t = t;
				rec.p = ray.at(rec.t);
				rec.u = 0;
				rec.v = 0;
				rec.normal = glm::normalize(rec.p - EvalBezier(localCPts, t, nullptr));
				rec.frontFace = true;
				rec.material = this->mat;
				hit = true;
				break;
			}

			inters.dt = min(inters.dt, 0.5f); /* Clamp to 0.5 */
			inters.dt = max(inters.dt, -0.5f); /* Clamp to -0.5 */

			dt1 = dt2;
			dt2 = inters.dt;

			if (dt1 * dt2 < 0.0f) { /* Summon the ancient Babylonian gods */

				float next = 0.0f;

				if ((iter & 3) == 0) {
					next = 0.5f * (t + tOld); /* Bisector method every 4th iteration */
				} else {
					next = (dt2 * tOld - dt1 * t) / (dt2 - dt1);
				}
				tOld = t;
				t = next;
			}
			else { /* Use the dt computed by the intersection; */
				tOld = t;
				t = t + inters.dt;
			}
			if (t < 0.0f || t > 1.0f) break;
		}

		if (!hit) tStart = 1.0f - tStart;
		else break;
	}
	return hit;
}

bool Curve::hitPBRT(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {

	glm::fvec3 localCPts[4];
	localCPts[0] = BlossomBezier(common->curvePoints, uMin, uMin, uMin);
	localCPts[1] = BlossomBezier(common->curvePoints, uMin, uMin, uMax);
	localCPts[2] = BlossomBezier(common->curvePoints, uMin, uMax, uMax);
	localCPts[3] = BlossomBezier(common->curvePoints, uMax, uMax, uMax);

	auto upVec = glm::cross(ray.getDirection(), glm::vec3(1, 0, 0));
	const auto objectToRay = glm::lookAt(ray.getOrigin(), ray.getOrigin() - ray.getDirection(), upVec);

	glm::fvec3 transformedPoints[4] = { objectToRay * glm::vec4(localCPts[0], 1), objectToRay * glm::vec4(localCPts[1], 1),
					  objectToRay * glm::vec4(localCPts[2], 1), objectToRay * glm::vec4(localCPts[3], 1) };

	float L0 = 0;
	for (int i = 0; i < 2; ++i) {
		L0 = std::max(L0,
			std::max(std::max(std::abs(transformedPoints[i].x - 2 * transformedPoints[i + 1].x + transformedPoints[i + 2].x),
				std::abs(transformedPoints[i].y - 2 * transformedPoints[i + 1].y + transformedPoints[i + 2].y)),
				std::abs(transformedPoints[i].z - 2 * transformedPoints[i + 1].z + transformedPoints[i + 2].z))
		);
	};
		
	float eps = std::max(common->width[0], common->width[1]) * .05f; // width / 20
#define LOG4(x) (std::log(x) * 0.7213475108f)
	float fr0 = LOG4(1.41421356237f * 12.f * L0 / (8.f * eps));
#undef LOG4
	int r0 = (int)std::round(fr0);
	int maxDepth = std::clamp(r0, 0, 10);

	auto inv = glm::inverse(objectToRay);
	return recursiveIntersect(ray, tMin, tMax, rec, transformedPoints, inv, uMin, uMax, maxDepth);
}

bool Curve::recursiveIntersect(const Ray& ray, float tMin, float tMax, HitRecord& rec, const glm::fvec3 cPts[4], glm::mat4x4& rayToObject, float u0, float u1, int depth) const {

	AABB curveBounds = { INF, INF, INF, -INF, -INF, -INF };

	for (int i = 0; i < 4; i++) {
		curveBounds.minX = min(curveBounds.minX, cPts[i].x);
		curveBounds.minY = min(curveBounds.minY, cPts[i].y);
		curveBounds.minZ = min(curveBounds.minZ, cPts[i].z);
		curveBounds.maxX = max(curveBounds.maxX, cPts[i].x);
		curveBounds.maxY = max(curveBounds.maxY, cPts[i].y);
		curveBounds.maxZ = max(curveBounds.maxZ, cPts[i].z);
	}


	float maxWidth = std::max(lerp(common->width[0], common->width[1], u0), lerp(common->width[0], common->width[1], u1));
	expandBBox(curveBounds, glm::fvec3(0.5 * maxWidth));

	float rayLength = glm::length(ray.getDirection());
	float zMax = rayLength * tMax;
	AABB rayBounds = { 0, 0, 0, 0, 0, zMax };

	if (overlaps(curveBounds, rayBounds) == false)
		return false;

	if (depth > 0) {
		float uMid = 0.5f * (u0 + u1);
		glm::fvec3 cpSplit[7];
		SubdivideBezier(cPts, cpSplit);
		return (recursiveIntersect(ray, tMin, tMax, rec, &cpSplit[0], rayToObject, u0, uMid, depth - 1) ||
				recursiveIntersect(ray, tMin, tMax, rec, &cpSplit[3], rayToObject, uMid, u1, depth - 1));
	}
	else {
		float edge = (cPts[1].y - cPts[0].y) * -cPts[0].y +
				cPts[0].x * (cPts[0].x - cPts[1].x);
		if (edge < 0)
			return false;

		edge = (cPts[2].y - cPts[3].y) * -cPts[3].y +
			cPts[3].x * (cPts[3].x - cPts[2].x);
		if (edge < 0)
			return false;


		glm::fvec2 segmentDirection = glm::fvec2(cPts[3]) - glm::fvec2(cPts[0]);
		float denom = glm::length(segmentDirection) * glm::length(segmentDirection);
		if (denom == 0)
			return false;
		float w = glm::dot(-glm::fvec2(cPts[0]), segmentDirection) / denom;


		float u = std::clamp(lerp(u0, u1, w), u0, u1);
		float hitWidth = lerp(common->width[0], common->width[1], u);
		glm::fvec3 nHit;


		glm::fvec3 dpcdw;
		glm::fvec3 pc = EvalBezier(cPts, std::clamp(w, 0.0f, 1.0f), &dpcdw);
		float ptCurveDist2 = pc.x * pc.x + pc.y * pc.y;
		if (ptCurveDist2 > hitWidth * hitWidth * .25f)
			return false;
		if (pc.z < 0 || pc.z > zMax)
			return false;

		float ptCurveDist = std::sqrt(ptCurveDist2);
		float edgeFunc = dpcdw.x * -pc.y + pc.x * dpcdw.y;
		float v = (edgeFunc > 0) ? 0.5f + ptCurveDist / hitWidth :
			0.5f - ptCurveDist / hitWidth;

		float tHit = pc.z / rayLength;

		if (tHit < tMin) {
			return false;
		}

		glm::fvec3 pError(2 * hitWidth, 2 * hitWidth, 2 * hitWidth);

		glm::fvec3 dpdu, dpdv;
		EvalBezier(common->curvePoints, u, &dpdu);
		glm::fvec3 dpduPlane = glm::inverse(rayToObject) * glm::fvec4(dpdu, 1);
		glm::fvec3 dpdvPlane = glm::normalize(glm::fvec3(-dpduPlane.y, dpduPlane.x, 0)) * hitWidth;
		float theta = lerp(-90.f, 90.f, v);
		auto rot = glm::rotate(glm::mat4x4(1.0f), -theta, dpduPlane);
		dpdvPlane = rot * glm::fvec4(dpdvPlane, 1);
		dpdv = rayToObject * glm::fvec4(dpdvPlane, 1);

		rec.p = ray.at(tHit);
		rec.t = tHit;
		rec.normal = -ray.getDirection();
		rec.u = u;
		rec.v = v;
		rec.material = mat;

		return true;
	}
}

void Curve::SubdivideBezier(const glm::fvec3 cp[4], glm::fvec3 cpSplit[7]) const {
	cpSplit[0] = cp[0];
	cpSplit[1] = (cp[0] + cp[1]) / 2.0f;
	cpSplit[2] = (cp[0] + 2.0f * cp[1] + cp[2]) / 4.0f;
	cpSplit[3] = (cp[0] + 3.0f * cp[1] + 3.0f * cp[2] + cp[3]) / 8.0f;
	cpSplit[4] = (cp[1] + 2.0f * cp[2] + cp[3]) / 4.0f;
	cpSplit[5] = (cp[2] + cp[3]) / 2.0f;
	cpSplit[6] = cp[3];
}

glm::fvec3 Curve::EvalBezier(const glm::fvec3 cp[4], float u, glm::fvec3* deriv = nullptr) const {
	glm::fvec3 cp1[3] = { lerp(cp[0], cp[1], u), lerp(cp[1], cp[2], u),
					   lerp(cp[2], cp[3], u) };
	glm::fvec3 cp2[2] = { lerp(cp1[0], cp1[1], u), lerp(cp1[1], cp1[2], u) };
	if (deriv)
		*deriv = 3.0f * (cp2[1] - cp2[0]);
	return lerp(cp2[0], cp2[1], u);
}

glm::fvec3 Curve::getTangent(const glm::fvec3 localCPts[4], float t) const {
	auto onemint = 1 - t;
	auto onemintSquared = onemint * onemint;
	auto a = -3.0f * onemintSquared * localCPts[0];
	auto b = localCPts[1] * (3.0f * onemintSquared - 6.0f * t * onemint);
	auto c = localCPts[2] * (-3.0f * t * t + 6.0f * t * onemint);
	auto d = localCPts[3] * 3.0f * t * t;
	return a + b + c + d;
}
