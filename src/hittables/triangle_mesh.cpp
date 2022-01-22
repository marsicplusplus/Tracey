#include "triangle_mesh.hpp"

#include <utility>

TriangleMesh::TriangleMesh(std::string name, unsigned int nTri, unsigned int nVerts, const unsigned int *vertexIndices, const glm::vec3 *P, const glm::vec3 *N, const glm::vec2 *UV) : 
	nTriangles{nTri}, nVertices{nVerts}, name{std::move(name)},
	vertexIndices(vertexIndices, vertexIndices + 3 * nTri) {
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
