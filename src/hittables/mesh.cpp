#include "hittables/mesh.hpp"

#include <iostream>
bool Mesh::hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const {
	bool ret = false;
	for(size_t i = 0; i < this->indices.size(); i+=3){
		glm::dvec3 v0 = this->pos[indices[i].vertex_index];
		glm::dvec3 v1 = this->pos[indices[i+1].vertex_index];
		glm::dvec3 v2 = this->pos[indices[i+2].vertex_index];
		double t, closest = tMax;
		if(intersect(v0, v1, v2, ray, t)){
			if(t < tMax && t > tMin && closest > t) {
				closest = t;
				rec.t = t;
				rec.p = ray.at(rec.t);
				rec.u = this->uvs[indices[i].texcoord_index].x;
				rec.v = this->uvs[indices[i].texcoord_index].y;
				rec.material = mat;
				rec.setFaceNormal(ray, this->norm[indices[i].normal_index]);
				ret = true;
			}
		}
	}
	return ret;
}

bool Mesh::intersect(const glm::dvec3 &v0, const glm::dvec3 &v1, const glm::dvec3 &v2, const Ray &ray, double &t) const {
		glm::dvec3 v0v1 = v1 - v0;
		glm::dvec3 v0v2 = v2 - v0;
		glm::dvec3 p = glm::cross(ray.getDirection(), v0v2);
		double det = glm::dot(v0v1, p);
		if(fabs(det) < 0.001) return false;
		double inv = 1.0/det;

		glm::dvec3 tv = ray.getOrigin() - v0;
		double u = glm::dot(tv, p) * inv;
		if(u < 0 || u > 1) return false;

		glm::dvec3 q = glm::cross(tv, v0v1);
		double v = glm::dot(ray.getDirection(), q) * inv;
		if(v < 0 || u + v > 1) return false;
		double tmp = glm::dot(v0v2, q) * inv;
		if(tmp < 0) return false;
		t = tmp;
		return true;
}
