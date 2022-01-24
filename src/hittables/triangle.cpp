#include "triangle.hpp"

Triangle::Triangle(const std::shared_ptr<TriangleMesh> &mesh, unsigned int triangleNumber, int material) : mesh(mesh), mat{material} {
	vIdx = &mesh->vertexIndices[triangleNumber * 3];
	glm::fvec3 *v0 = &(this->mesh->p.get())[vIdx[0]];
	glm::fvec3 *v1 = &(this->mesh->p.get())[vIdx[1]];
	glm::fvec3 *v2 = &(this->mesh->p.get())[vIdx[2]];
	const glm::vec3 xVal = {v0->x, v1->x, v2->x};
	const glm::vec3 yVal = {v0->y, v1->y, v2->y};
	const glm::vec3 zVal = {v0->z, v1->z, v2->z};
	localBBox.minX = min(min(xVal.x, xVal.y), xVal.z);
	localBBox.minY = min(min(yVal.x, yVal.y), yVal.z);
	localBBox.minZ = min(min(zVal.x, zVal.y), zVal.z);
	localBBox.maxX = max(max(xVal.x, xVal.y), xVal.z);
	localBBox.maxY = max(max(yVal.x, yVal.y), yVal.z);
	localBBox.maxZ = max(max(zVal.x, zVal.y), zVal.z);
	if(localBBox.minX == localBBox.maxX){
		localBBox.minX -= 0.0001f;
		localBBox.maxX += 0.0001f;
	}
	if(localBBox.minY == localBBox.maxY){
		localBBox.minY -= 0.0001f;
		localBBox.maxY += 0.0001f;
	}
	if(localBBox.minZ == localBBox.maxZ){
		localBBox.minZ -= 0.0001f;
		localBBox.maxZ += 0.0001f;
	}
	updateWorldBBox();
}

void Triangle::getUV(glm::vec2 uv[3]) const {
	if(mesh->uv){
		uv[0] = mesh->uv[vIdx[0]];
		uv[1] = mesh->uv[vIdx[1]];
		uv[2] = mesh->uv[vIdx[2]];
	} else {
		uv[0] = glm::fvec2{0, 0};
		uv[1] = glm::fvec2{1, 0};
		uv[2] = glm::fvec2{1, 1};
	}
}

bool Triangle::hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
	auto transformInv = transform.getInverse();
	auto transposeInv = transform.getTransposeInverse();
	auto transformMat = transform.getMatrix();
	const auto transformedRay = ray.transformRay(transformInv);

	glm::fvec3 *v0 = &(this->mesh->p.get())[vIdx[0]];
	glm::fvec3 *v1 = &(this->mesh->p.get())[vIdx[1]];
	glm::fvec3 *v2 = &(this->mesh->p.get())[vIdx[2]];

	glm::fvec3 v0v1 = *v1 - *v0;
	glm::fvec3 v0v2 = *v2 - *v0;
	glm::fvec3 p = glm::cross(transformedRay.getDirection(), v0v2);
	float det = glm::dot(v0v1, p);
	if (std::fabs(det) < EPS) return false;
	float inv = 1.0f / det;

	glm::fvec3 tv = transformedRay.getOrigin() - *v0;
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

		glm::fvec3 *n0 = &(this->mesh->n.get())[vIdx[0]];
		glm::fvec3 *n1 = &(this->mesh->n.get())[vIdx[1]];
		glm::fvec3 *n2 = &(this->mesh->n.get())[vIdx[2]];

		glm::fvec3 hitNormal;
		hitNormal = u * (*n1) + v * (*n2) + (1.0f - u - v) * (*n0);

		rec.setFaceNormal(ray, transposeInv * glm::fvec4(hitNormal, 0.0));

		glm::fvec2 uvs[3];
		getUV(&uvs[0]);

		glm::fvec2 uv = u * (uvs[1]) + v * (uvs[2]) + (1.0f - u - v) * (uvs[0]);
		
		rec.u = uv.x;
		rec.v = uv.y;
		rec.material = mat;
		rec.p = transformMat * glm::fvec4(localp, 1.0);
		rec.t = tmp;

		return true;
	}

	return false;
}

void Triangle::copyTri(CompactTriangle *ct) {
	ct->mat = this->mat;
	ct->v0 = glm::vec4((this->mesh->p.get())[vIdx[0]], 0);
	ct->v1 = glm::vec4((this->mesh->p.get())[vIdx[1]], 0);
	ct->v2 = glm::vec4((this->mesh->p.get())[vIdx[2]], 0);
													 
	ct->n0 = glm::vec4((this->mesh->n.get())[vIdx[0]], 0);
	ct->n1 = glm::vec4((this->mesh->n.get())[vIdx[1]], 0);
	ct->n2 = glm::vec4((this->mesh->n.get())[vIdx[2]], 0);

	if(mesh->uv){
		ct->u0 = glm::fvec4(mesh->uv[vIdx[0]], 0,0);
		ct->u1 = glm::fvec4(mesh->uv[vIdx[1]], 0,0);
		ct->u2 = glm::fvec4(mesh->uv[vIdx[2]], 0,0);
	} else {
		ct->u0 = glm::fvec4{0, 0,0,0};
		ct->u1 = glm::fvec4{1, 0,0,0};
		ct->u2 = glm::fvec4{1, 1,0,0};
	}

}
