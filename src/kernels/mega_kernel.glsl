layout(local_size_x=32, local_size_y=32) in;

struct triangle {
	int matIdx;
	vec2 uv0, uv1, uv2;
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
};

layout(std430, binding = 0) readonly buffer Meshes {
	triangle[] triangles;
};

struct instance{
	int first;
	int count;
	mat4 transform;
};

layout(std430, binding = 1) readonly buffer Instances {
	instance[] instances;
};

struct Node {
	vec4 minAABBLeftFirst;
	vec4 maxAABBCount;
};

layout(std430, binding = 2) readonly buffer Meshes {
	
};

int main() {
}
