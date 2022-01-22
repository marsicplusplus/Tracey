#ifndef __TRIANGLE_MESH_HPP__
#define __TRIANGLE_MESH_HPP__

#include <vector>
#include <memory>
#include <string>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"

struct Instance {
	int meshIdx;
	glm::mat4x4 transformMat;
	glm::mat4x4 transformInv;
	glm::mat4x4 transposeInv;
};

struct CompactMesh {
	int firstTri;
	int lastTri;
	int firstNode;
	int lastNode;
};

class TriangleMesh {
	public:
		TriangleMesh(std::string  name, unsigned int nTri, unsigned int nVerts, const unsigned int *vertexIndices, const glm::vec3 *p, const glm::vec3 *n, const glm::vec2 *uv);

		const unsigned int nTriangles, nVertices;
		std::vector<unsigned int> vertexIndices;
		std::unique_ptr<glm::vec3[]> p;
		std::unique_ptr<glm::vec3[]> n;
		std::unique_ptr<glm::vec2[]> uv;
		const std::string name;
};

#endif
