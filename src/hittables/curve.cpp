#include "hittables/curve.hpp"

Curve::Curve(float uMin, float uMax, int material) : mat(material), uMin(uMin), uMax(uMax) {

}

bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) {
	return false;
}
