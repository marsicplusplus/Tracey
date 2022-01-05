#include "triangle_mesh.hpp"

TriangleMesh::TriangleMesh(const std::string &name, unsigned int nTri, unsigned int nVerts, const unsigned int *vertexIndices, const glm::vec3 *P, const glm::vec3 *N, const glm::vec2 *UV) : 
	nTriangles{nTri}, nVertices{nVerts}, name{name},
	vertexIndices(vertexIndices, vertexIndices + 3 * nVerts) {
	p.reset(new glm::vec3[nVertices]);
	for(int i = 0; i < nVertices; ++i){
		p[i] = P[i];
	}
	if(UV){
		uv.reset(new glm::vec2[nVertices]);
		for(int i = 0; i < nVertices; ++i){
			uv[i] = UV[i];
		}
	}
	if(N){
		n.reset(new glm::vec3[nVertices]);
		for(int i = 0; i < nVertices; ++i){
			n[i] = N[i];
		}
	}
}
