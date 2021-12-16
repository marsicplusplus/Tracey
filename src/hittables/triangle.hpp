#ifndef __TRIANGLE_HPP__
#define __TRIANGLE_HPP__

#include "defs.hpp"
#include "bvh.hpp"
#include "glm/vec3.hpp"
#include "hittables/hittable.hpp"
#include "glm/gtx/norm.hpp"
#include "tiny_obj_loader.h"

struct Triangle : Hittable {
	public:
		Triangle(
			glm::ivec3 fIdx,
			glm::ivec3 nIdx, 
			glm::ivec3 tIdx, 
			int m,
			std::vector<glm::fvec3>& pos, 
			std::vector<glm::fvec3>& norm,
			std::vector<glm::fvec2>& uvs
		) : 
			face{fIdx}, 
			normal{nIdx},
			texture{tIdx}, 
			mat{m} {
			
			v0 = pos[face.x];
			v1 = pos[face.y];
			v2 = pos[face.z];

			const glm::fvec3& xVal = {v0.x, v1.x, v2.x};
			const glm::fvec3& yVal = {v0.y, v1.y, v2.y};
			const glm::fvec3& zVal = {v0.z, v1.z, v2.z};

			if (normal.x == -1 || normal.y == -1 || normal.z == -1) {
				n0 = { 0.0f, 0.0f, 0.0f };
				n1 = { 0.0f, 0.0f, 0.0f };
				n2 = { 0.0f, 0.0f, 0.0f };
			} else {
				n0 = norm[normal.x];
				n1 = norm[normal.y];
				n2 = norm[normal.z];
			}

			if (texture.x == -1 || texture.y == -1 || texture.z == -1) {
				st0 = { 0.0f, 0.0f };
				st1 = { 0.0f, 0.0f };
				st2 = { 0.0f, 0.0f };
			}
			else {
				st0 = uvs[texture.x];
				st1 = uvs[texture.y];
				st2 = uvs[texture.z];
			}

			localBBox.minX = min(min(xVal.x, xVal.y), xVal.z);
			localBBox.minY = min(min(yVal.x, yVal.y), yVal.z);
			localBBox.minZ = min(min(zVal.x, zVal.y), zVal.z);
			localBBox.maxX = max(max(xVal.x, xVal.y), xVal.z);
			localBBox.maxY = max(max(yVal.x, yVal.y), yVal.z);
			localBBox.maxZ = max(max(zVal.x, zVal.y), zVal.z);
			updateWorldBBox();
		}

		inline bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override {

			const auto transformedRay = ray.transformRay(transformInv);

			glm::fvec3 v0v1 = v1 - v0;
			glm::fvec3 v0v2 = v2 - v0;
			glm::fvec3 p = glm::cross(transformedRay.getDirection(), v0v2);
			float det = glm::dot(v0v1, p);
			if (std::fabs(det) < 0.00001f) return false;
			float inv = 1.0f / det;

			glm::fvec3 tv = transformedRay.getOrigin() - v0;
			float u = glm::dot(tv, p) * inv;
			if (u < 0.0f || u > 1.0f) return false;

			glm::fvec3 q = glm::cross(tv, v0v1);
			float v = glm::dot(transformedRay.getDirection(), q) * inv;
			if (v < 0.0f || u + v > 1.0f) return false;
			float tmp = glm::dot(v0v2, q) * inv;
			if (tmp < 0.0f) return false;

			if (tmp > tMin && tmp < tMax) {
				rec.t = tmp;
				auto localp = transformedRay.at(tmp);

				glm::fvec3 hitNormal;
				if (normal.x == -1 || normal.y == -1 || normal.z == -1)
					hitNormal = glm::cross(v0v1, v0v2);
				else {
					hitNormal = u * n1 + v * n2 + (1.0f - u - v) * n0;
				}

				rec.setFaceNormal(ray, transposeInv * glm::fvec4(hitNormal, 0.0));

				glm::fvec2 uv;
				if (texture.x == -1 || texture.y == -1 || texture.z == -1) {
					uv = glm::fvec2{ 0,0 };
				}
				else {
					uv = u * st1 + v * st2 + (1.0f - u - v) * st0;
				}
				rec.u = uv.x;
				rec.v = uv.y;
				rec.material = mat;
				rec.p = transform * glm::fvec4(localp, 1.0);
				rec.t = tmp;

				return true;
			}

			return false;
		}

		glm::ivec3 face;
		glm::ivec3 normal;
		glm::ivec3 texture;

		glm::fvec3 v0;
		glm::fvec3 v1;
		glm::fvec3 v2;

		glm::fvec3 n0;
		glm::fvec3 n1;
		glm::fvec3 n2;

		glm::fvec2 st0;
		glm::fvec2 st1;
		glm::fvec2 st2;

		int mat;
};

#endif
