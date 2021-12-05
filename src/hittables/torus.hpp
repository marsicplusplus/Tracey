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
	Torus(double _radiusMajor, double _radiusMinor, glm::dmat4x4 _transform, MaterialPtr mat) : transform{_transform}, transformInv{glm::inverse(_transform)}, mat{mat} {
		//if (_radiusMinor > _radiusMajor) {
		//	auto tmp = _radiusMajor;
		//	_radiusMajor = _radiusMinor;
		//	_radiusMinor = tmp;
		//}

		radiusMajorSquared = _radiusMajor * _radiusMajor;
		radiusMinorSquared = _radiusMinor * _radiusMinor;
	}

	inline bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
		const auto transformedRay = ray.transformRay(transformInv);
		const auto transformedRayDir = transformedRay.getDirection();
		const auto transformedOrigin = transformedRay.getOrigin();

		double rayDirDot = glm::dot(transformedRayDir, transformedRayDir);
		double originDot = glm::dot(transformedOrigin, transformedOrigin);

		double originDirDot = glm::dot(transformedRayDir, transformedOrigin);
		double commonTerm = originDot - radiusMajorSquared - radiusMinorSquared;

		double c4 = rayDirDot * rayDirDot;
		double c3 = 4.0 * rayDirDot * originDirDot;
		double c2 = 2.0 * rayDirDot * commonTerm + 4.0 * originDirDot * originDirDot + 4.0 * radiusMajorSquared * transformedRayDir.y * transformedRayDir.y;
		double c1 = 4.0 * commonTerm * originDirDot + 8.0 * radiusMajorSquared * transformedOrigin.y * transformedRayDir.y;
		double c0 = commonTerm * commonTerm - 4.0 * radiusMajorSquared * (radiusMinorSquared - transformedOrigin.y * transformedOrigin.y);

		c3 /= c4;
		c2 /= c4;
		c1 /= c4;
		c0 /= c4;

		std::complex<double>* solutions = solve_quartic(c3, c2, c1, c0);

		double minRealRoot = INF;
		if (solutions[0].real() > 0.0 && solutions[0].imag() == 0.0 && solutions[0].real() < minRealRoot) {
			minRealRoot = solutions[0].real();
		}

		if (solutions[1].real() > 0.0 && solutions[1].imag() == 0.0 && solutions[1].real() < minRealRoot) {
			minRealRoot = solutions[1].real();
		}

		if (solutions[2].real() > 0.0 && solutions[2].imag() == 0.0 && solutions[2].real() < minRealRoot) {
			minRealRoot = solutions[2].real();
		}

		if (solutions[3].real() > 0.0 && solutions[3].imag() == 0.0 && solutions[3].real() < minRealRoot) {
			minRealRoot = solutions[3].real();
		}

		delete[] solutions;

		if (minRealRoot == INF) {
			return false;
		}

		if ((minRealRoot < tMax) && (minRealRoot > tMin)) {

			auto localp = transformedRay.at(minRealRoot);
			double sumSquared = glm::dot(localp, localp);
			double radii = radiusMajorSquared + radiusMinorSquared;
			glm::dvec3 normal = glm::normalize(glm::dvec3(
				4.0 * localp.x * (sumSquared - radii),
				4.0 * localp.y * (sumSquared - radii + 2.0 * radiusMajorSquared),
				4.0 * localp.z * (sumSquared - radii)
			));

			auto transformedNormal = glm::inverse(transformInv) * glm::dvec4(normal, 0);

			rec.t = minRealRoot;
			rec.p = ray.at(minRealRoot);
			rec.material = mat;
			rec.setFaceNormal(ray, transformedNormal);
			getUV(rec);
			return true;
		}

		return false;
	}

	inline void getUV(HitRecord& rec) const {
		rec.u = 0;
		rec.v = 0;
	}

private:
	glm::dmat4x4 transformInv;
	glm::dmat4x4 transform;
	double radiusMajorSquared;// Distance from denter of tube to center of torus
	double radiusMinorSquared;// Radius of the Tube
	MaterialPtr mat;
};

#endif
