#ifndef __TORUS_HPP__
#define __TORUS_HPP__

#include "defs.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"
#include "quartic.h"

class Torus : public Hittable {
public:
	Torus(float _radiusMajor, float _radiusMinor, glm::fmat4x4 _transform, MaterialPtr mat) : transform{_transform}, transformInv{glm::inverse(_transform)}, mat{mat} {
		//if (_radiusMinor > _radiusMajor) {
		//	auto tmp = _radiusMajor;
		//	_radiusMajor = _radiusMinor;
		//	_radiusMinor = tmp;
		//}
		radiusMajor = _radiusMajor;
		radiusMajorSquared = _radiusMajor * _radiusMajor;
		radiusMinorSquared = _radiusMinor * _radiusMinor;
	}

	inline bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override {
		const auto transformedRay = ray.transformRay(transformInv);
		const auto transformedRayDir = transformedRay.getDirection();
		const auto transformedOrigin = transformedRay.getOrigin();

		float rayDirDot = glm::dot(transformedRayDir, transformedRayDir);
		float originDot = glm::dot(transformedOrigin, transformedOrigin);

		float originDirDot = glm::dot(transformedRayDir, transformedOrigin);
		float commonTerm = originDot - radiusMajorSquared - radiusMinorSquared;

		float c4 = rayDirDot * rayDirDot;
		float c3 = 4.0f * rayDirDot * originDirDot;
		float c2 = 2.0f * rayDirDot * commonTerm + 4.0f * originDirDot * originDirDot + 4.0f * radiusMajorSquared * transformedRayDir.y * transformedRayDir.y;
		float c1 = 4.0f * commonTerm * originDirDot + 8.0f * radiusMajorSquared * transformedOrigin.y * transformedRayDir.y;
		float c0 = commonTerm * commonTerm - 4.0f * radiusMajorSquared * (radiusMinorSquared - transformedOrigin.y * transformedOrigin.y);

		c3 /= c4;
		c2 /= c4;
		c1 /= c4;
		c0 /= c4;

		std::complex<double>* solutions = solve_quartic(c3, c2, c1, c0);

		float minRealRoot = INF;
		if (solutions[0].real() > 0.0f && solutions[0].imag() == 0.0f && solutions[0].real() < minRealRoot) {
			minRealRoot = solutions[0].real();
		}

		if (solutions[1].real() > 0.0f && solutions[1].imag() == 0.0f && solutions[1].real() < minRealRoot) {
			minRealRoot = solutions[1].real();
		}

		if (solutions[2].real() > 0.0f && solutions[2].imag() == 0.0f && solutions[2].real() < minRealRoot) {
			minRealRoot = solutions[2].real();
		}

		if (solutions[3].real() > 0.0f && solutions[3].imag() == 0.0f && solutions[3].real() < minRealRoot) {
			minRealRoot = solutions[3].real();
		}

		delete[] solutions;

		if (minRealRoot == INF) {
			return false;
		}

		if ((minRealRoot < tMax) && (minRealRoot > tMin)) {

			auto localp = transformedRay.at(minRealRoot);
			float sumSquared = glm::dot(localp, localp);
			float radii = radiusMajorSquared + radiusMinorSquared;
			glm::fvec3 normal = glm::normalize(glm::fvec3(
				4.0f * localp.x * (sumSquared - radii),
				4.0f * localp.y * (sumSquared - radii + 2.0f * radiusMajorSquared),
				4.0f * localp.z * (sumSquared - radii)
			));

			auto transformedNormal = glm::transpose(transformInv) * glm::fvec4(normal, 0);

			rec.t = minRealRoot;
			rec.p = transformInv * glm::fvec4(transformedRay.at(minRealRoot), 1);
			rec.material = mat;
			rec.setFaceNormal(ray, transformedNormal);
			getUV(rec);
			return true;
		}

		return false;
	}

	inline void getUV(HitRecord& rec) const {
		rec.u = 0.5f + atan2(rec.p.z, rec.p.x) / (2.0f*PI);
  		rec.v = 0.5f + atan2(rec.p.y, (sqrt(rec.p.x * rec.p.x + rec.p.z * rec.p.z) - radiusMajor)) / (2.0f * PI);
	}

private:
	glm::fmat4x4 transformInv;
	glm::fmat4x4 transform;
	float radiusMajor;// Distance from center of tube to center of torus
	float radiusMajorSquared;// Distance from center of tube to center of torus
	float radiusMinorSquared;// Radius of the Tube
	MaterialPtr mat;
};

#endif
