#include "hittables/mesh.hpp"
#include <iostream>

bool Mesh::hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const {
	bool ret = false;
	float closest = tMax;
	const Ray transformedRay = ray.transformRay(transformInv);

	if (rec.p != glm::fvec3{INF, INF, INF}) {
		tMax = glm::distance(transformedRay.getOrigin(), glm::fvec3(transformInv * glm::fvec4(rec.p, 1.0f)));
	}

	if (!hitAABB(transformedRay)) {
		return false;
	}

	for(auto &tri : this->tris){
		if(intersectTri(tri, transformedRay, ray, rec, tMin, closest)){
			closest = rec.t;
			ret = true;
		}
	}
	return ret;
}

bool Mesh::intersectTri(const Triangle &tri, const Ray &transformedRay, const Ray &ray, HitRecord &rec, float tMin, float tMax) const {
		const glm::fvec3 &v0 = this->pos[tri.face.x];
		const glm::fvec3 &v1 = this->pos[tri.face.y];
		const glm::fvec3 &v2 = this->pos[tri.face.z];

		glm::fvec3 v0v1 = v1 - v0;
		glm::fvec3 v0v2 = v2 - v0;
		glm::fvec3 p = glm::cross(transformedRay.getDirection(), v0v2);
		float det = glm::dot(v0v1, p);
		if(std::fabs(det) < 0.00001f) return false;
		float inv = 1.0f/det;

		glm::fvec3 tv = transformedRay.getOrigin() - v0;
		float u = glm::dot(tv, p) * inv;
		if(u < 0.0f || u > 1.0f) return false;

		glm::fvec3 q = glm::cross(tv, v0v1);
		float v = glm::dot(transformedRay.getDirection(), q) * inv;
		if(v < 0.0f || u + v > 1.0f) return false;
		float tmp = glm::dot(v0v2, q) * inv;
		if(tmp < 0.0f) return false;

		if(tmp > tMin && tmp < tMax){
			auto localp = transformedRay.at(tmp);

			glm::fvec3 hitNormal;
			if(tri.normal.x == -1 || tri.normal.y == -1 || tri.normal.z == -1)
				hitNormal = glm::cross(v0v1, v0v2);
			else{
				const glm::fvec3 &n0 = this->norm[tri.normal.x];
				const glm::fvec3 &n1 = this->norm[tri.normal.y];
				const glm::fvec3 &n2 = this->norm[tri.normal.z];
				hitNormal = u * n1 + v * n2 + (1.0f - u - v) * n0;
			}
			hitNormal = transposeInv * glm::fvec4(hitNormal, 0.0);
			rec.setFaceNormal(ray, hitNormal);

			glm::fvec2 uv;
			if(tri.texture.x == -1 || tri.texture.y == -1 || tri.texture.z == -1){
				uv = glm::fvec2{0,0};
			}
			else {
				const glm::fvec2 &st0 = this->uvs[tri.texture.x];
				const glm::fvec2 &st1 = this->uvs[tri.texture.y];
				const glm::fvec2 &st2 = this->uvs[tri.texture.z];
				uv = u * st1 + v * st2 + (1.0f - u - v) * st0;
			}
			rec.u = uv.x;
			rec.v = uv.y;
			rec.material = tri.mat;
			rec.p = transform * glm::fvec4(localp, 1.0);
			rec.t = glm::distance(rec.p, ray.getOrigin());

			return true;
		}
		
		return false;
}
