#include "hittables/mesh.hpp"
#include <iostream>

bool Mesh::hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const {
	bool ret = false;
	double closest = tMax;
	for(auto &tri : this->tris){
		if(intersectTri(tri, ray, rec)){
			if(rec.t < tMax && rec.t > tMin && rec.t < closest){
				closest = rec.t;
				ret = true;
			}
		}
	}
	return ret;
}

bool Mesh::intersectTri(const Triangle &tri, const Ray &ray, HitRecord &rec) const {
		const glm::dvec3 &v0 = this->pos[tri.face.x];
		const glm::dvec3 &v1 = this->pos[tri.face.y];
		const glm::dvec3 &v2 = this->pos[tri.face.z];

		glm::dvec3 v0v1 = v1 - v0;
		glm::dvec3 v0v2 = v2 - v0;
		glm::dvec3 p = glm::cross(ray.getDirection(), v0v2);
		double det = glm::dot(v0v1, p);
		if(det < 0.00001) return false;
		double inv = 1.0/det;

		glm::dvec3 tv = ray.getOrigin() - v0;
		double u = glm::dot(tv, p) * inv;
		if(u < 0.0 || u > 1.0) return false;

		glm::dvec3 q = glm::cross(tv, v0v1);
		double v = glm::dot(ray.getDirection(), q) * inv;
		if(v < 0 || u + v > 1) return false;
		double tmp = glm::dot(v0v2, q) * inv;
		if(tmp < 0) return false;

		rec.t = tmp;
		rec.p = ray.at(tmp);

		glm::dvec3 hitNormal;
		if(tri.normal.x == -1 || tri.normal.y == -1 || tri.normal.z == -1)
			hitNormal = glm::cross((v1 - v0), (v2 - v0));
		else{
			const glm::dvec3 &n0 = this->norm[tri.normal.x];
			const glm::dvec3 &n1 = this->norm[tri.normal.y];
			const glm::dvec3 &n2 = this->norm[tri.normal.z];
			hitNormal = u * n1 + v * n2 + (1.0 - u - v) * n0; 
		}
		rec.setFaceNormal(ray, (hitNormal));

		glm::dvec2 uv;
		if(tri.texture.x == -1 || tri.texture.y == -1 || tri.texture.z == -1){
			uv = glm::dvec2{0,0};
		} else { 
			const glm::dvec2 &st0 = this->uvs[tri.texture.x];
			const glm::dvec2 &st1 = this->uvs[tri.texture.y];
			const glm::dvec2 &st2 = this->uvs[tri.texture.z];
			uv = u * st0 + v * st1 + (1.0 - u - v) * st2; 
		}
		rec.u = uv.x;
		rec.v = uv.y;
		rec.material = tri.mat;

		return true;
}
