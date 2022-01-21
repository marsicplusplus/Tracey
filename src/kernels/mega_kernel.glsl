#version 450 core

layout (local_size_x=32, local_size_y=32) in;

layout(binding = 0, rgba32f) uniform image2D framebufferImage;

const float INFINITY = 1. / 0.;
const float EPSILON = 1e-10;
const uint NILL = 0x00000000u;

/*******************************************************************

MESH AND INSTANCE DEFINITION

********************************************************************/

struct Mesh {
	int firstTri;
	int lastTri;
	int firstNode;
	int lastNode;
};

layout(std430, binding = 1) readonly buffer Meshes {
	Mesh[] meshes;
};

struct Instance{
	int meshIdx;
	mat4 transformMat;
	mat4 transformInv;
	mat4 transposeInv;
};

layout(std430, binding = 2) readonly buffer Instances {
	Instance[] instances;
};


/*******************************************************************

TEXTURE DEFINITION AND METHODS

********************************************************************/

const uint SOLID = 0x00000001u;
const uint IMAGE = 0x00000002u;
const uint CHECKERED = 0x00000004u;

struct Texture {
	uint type;
	int idx;
	int width;
	int height;
	int slice;
	int bpp;
	vec3 color1;
	vec3 color2;
};

layout(std430, binding = 3) readonly buffer Textures {
	Texture[] textures;
};

vec3 getTextureColor(int textureIdx, float u, float v, const vec3 p) {
	Texture tex = textures[textureIdx];
	return tex.color1;
//	if (tex.type == SOLID) {
//		return color1;
//	} else if (tex.type == IMAGE) {
//		return color1;
//	} else {
//		return color1;
//	}
}



/*******************************************************************

HIT RECORD AND RAY DEFINITION/METHODS

********************************************************************/

struct HitRecord {
	bool frontFace;
	int material;
	float u;
	float v;
	float t;
	vec3 p;
	vec3 normal;
};

struct Ray {
	vec3 origin;
	vec3 direction;
	vec3 invDirection;
};

Ray transformRay(Ray origRay, mat4 transform) {
	vec4 newDir = transform * vec4(origRay.direction, 0);
	vec4 newOg = transform * vec4(origRay.origin, 1);
	Ray ret;
	ret.origin = vec3(newOg);
	ret.direction = vec3(newDir);
	ret.invDirection = 1.0 / ret.direction;
	return ret;
}


bool hitAABB(Ray ray, vec3 minAABB, vec3 maxAABB, out float distance) {
	float tmin = -INFINITY, tmax = INFINITY;

	float tx1 = (minAABB.x - ray.origin.x) * ray.invDirection.x;
	float tx2 = (maxAABB.x - ray.origin.x) * ray.invDirection.x;

	tmin = max(tmin, min(tx1, tx2));
	tmax = min(tmax, max(tx1, tx2));

	float ty1 = (minAABB.y - ray.origin.y) * ray.invDirection.y;
	float ty2 = (maxAABB.y - ray.origin.y) * ray.invDirection.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (minAABB.z - ray.origin.z) * ray.invDirection.z;
	float tz2 = (maxAABB.z - ray.origin.z) * ray.invDirection.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	distance = max(tmin, 0.0);
	return (tmax >= distance);
}



void setFaceNormal(HitRecord rec, const Ray r, const vec3 outNormal) {
	rec.frontFace = dot(r.direction, outNormal) < 0.0;
	rec.normal = rec.frontFace ? normalize(outNormal) : normalize(-outNormal);
}



/*******************************************************************

TRIANGLE DEFINITON AND METHODS

********************************************************************/

struct Triangle {
	int matIdx;
	vec2 uv0, uv1, uv2;
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
};


layout(std430, binding = 4) readonly buffer Triangles {
	Triangle[] triangles;
};

bool hitTriangle(Ray ray, Triangle tri, float tMin, float tMax, HitRecord rec) {

	vec3 v0v1 = tri.v1 - tri.v0;
	vec3 v0v2 = tri.v2 - tri.v0;
	vec3 p = cross(ray.direction, v0v2);
	float det = dot(v0v1, p);
	if (abs(det) < EPSILON) return false;
	float inv = 1.0 / det;

	vec3 tv = ray.origin - tri.v0;
	float u = dot(tv, p) * inv;
	if (u < 0.0 || u > 1.0) return false;

	vec3 q = cross(tv, v0v1);
	float v = dot(ray.direction, q) * inv;
	if (v < 0.0 || u + v > 1.0) return false;
	float tmp = dot(v0v2, q) * inv;
	if (tmp < 0.0) return false;

	if (tmp > tMin && tmp < tMax) {

		vec3 hitNormal = u * (tri.n1) + v * (tri.n2) + (1.0 - u - v) * (tri.n0);

		setFaceNormal(rec, ray, hitNormal);

		vec2 uv = u * (tri.uv1) + v * (tri.uv2) + (1.0 - u - v) * (tri.uv0);
		
		rec.u = uv.x;
		rec.v = uv.y;
		rec.material = tri.matIdx;
		rec.p = ray.origin + tmp * ray.direction;
		rec.t = tmp;

		return true;
	}

	return false;
}

/*******************************************************************

BVH DEFINITON AND METHODS

********************************************************************/

struct Node {
	vec3 minAABB;
	vec3 maxAABB;
	int leftFirst;
	int count;

};


layout(std430, binding = 5) readonly buffer BVHNodes {
	Node[] nodes;
};

bool traverseBVH(Ray ray, Instance instance, out float tMin, out float tMax, out HitRecord rec) {
	HitRecord tmp;
	bool hasHit = false;
	float closest = tMax;

	int meshIdx = instance.meshIdx;
	Mesh mesh = meshes[meshIdx];
	int firstTri = mesh.firstTri;
	int firstNode = mesh.firstNode;

	Node nodestack[64];
	int stackPtr = 0;
	nodestack[stackPtr++] = nodes[firstNode]; // Push the root into the stack

	while(stackPtr != 0){
		Node currNode = nodestack[--stackPtr];
		if(currNode.count != 0) {// I'm a leaf
			for (int i = firstTri + int(currNode.leftFirst); i < firstTri + currNode.leftFirst + currNode.count; ++i) {
				Triangle tri = triangles[i];
				if (hitTriangle(ray, tri, tMin, closest, tmp)) {
					rec = tmp;
					closest = rec.t;
					hasHit = true;
				}
			}
			tMax = closest;
		} else {
			int idx1 = firstNode + currNode.leftFirst;
			int idx2 = firstNode + currNode.leftFirst + 1;
			Node firstNode = nodes[idx1];
			Node secondNode = nodes[idx2];

			float firstDistance = 0.0;
			float secondDistance = 0.0;

			bool hitAABBFirst = hitAABB(ray, firstNode.minAABB, firstNode.maxAABB, firstDistance);
			bool hitAABBSecond = hitAABB(ray, secondNode.minAABB, secondNode.maxAABB, secondDistance);

			if (hitAABBFirst && hitAABBSecond) {
				if(firstDistance < secondDistance && firstDistance < tMax){
					nodestack[stackPtr++] = secondNode;
					nodestack[stackPtr++] = firstNode;
				} else if (secondDistance < tMax) {
					nodestack[stackPtr++] = firstNode;
					nodestack[stackPtr++] = secondNode;
				}
			} else if(hitAABBFirst && firstDistance < tMax){
				nodestack[stackPtr++] = firstNode;
			} else if(hitAABBSecond && secondDistance < tMax) {
				nodestack[stackPtr++] = secondNode;
			}
		}
	}
	return hasHit;
}

bool intersectBVH(Ray ray, Instance instance, float tMin, float tMax, out HitRecord rec) {
	mat4 transformInv = instance.transformInv;
	mat4 transposeInv = instance.transposeInv;
	mat4 transformMat = instance.transformMat;
	Ray transformedRay = transformRay(ray, transformInv);

	HitRecord tmp;
	if (traverseBVH(transformedRay, instance, tMin, tMax, tmp)) {
		rec = tmp;
		rec.p = vec3(transformMat * vec4(rec.p, 1.0));
		setFaceNormal(rec, ray, vec3(transposeInv * vec4(rec.normal, 0.0)));
		return true;
	}

	return false;
}

/*******************************************************************

MATERIAL DEFINITON AND METHODS

********************************************************************/

const uint DIFFUSE = 0x00000001u;
const uint DIELECTRIC  = 0x00000002u;
const uint MIRROR  = 0x00000004u;

struct Material {
	uint type;
	int albedoIdx;
	int bump;
	float reflectionIdx;
	vec3 absorption;
	float ior;
};

layout(std430, binding = 6) readonly buffer Materials {
	Material[] materials;
};


void material_reflect(Material mat, Ray ray, HitRecord hr, out Ray reflectedRay, out float reflectance) {
	if (mat.type == MIRROR) {
		reflectance = mat.reflectionIdx;
	} else if (mat.type == DIELECTRIC) {
		float n1 = (hr.frontFace) ? 1.0 : mat.ior;
		float n2 = (hr.frontFace) ? mat.ior : 1.0;
		float cosThetai = dot(-ray.direction, hr.normal);
		float ratio = n1/n2;
		float k = 1.0 - (ratio * ratio) * (1.0 - (cosThetai * cosThetai));
		if (k < 0.0){
			/* TIR */
			reflectance = 1.0;
		} else {
			/* Fresnel please help me */
			float sinThetai = sqrt(1.0 - (cosThetai * cosThetai));
			float cosThetat = sqrt(1.0 - ((ratio * sinThetai) * (ratio * sinThetai)));
			float rs = ((n1 * cosThetai - n2 * cosThetat) / (n1 * cosThetai + n2 * cosThetat));
			float rp = ((n1 * cosThetat - n2 * cosThetai) / (n1 * cosThetat + n2 * cosThetai));
			reflectance = (rs*rs+rp*rp)/2.0;
		}
	}

	vec3 reflectedDir = reflect(ray.direction, hr.normal);
	reflectedRay.origin = hr.p + 0.001 * reflectedDir;
	reflectedRay.direction = reflectedDir;
	reflectedRay.invDirection = 1.0 / reflectedDir;
}

void material_refract(Ray ray, HitRecord hr, float ior, out Ray refractedRay, out float refractance) {
	float cosi = dot(ray.direction, hr.normal);
	float n1 = hr.frontFace ? 1.0 : ior;
	float n2 = hr.frontFace ? ior : 1.0;
	float ratio = n1/n2;
	float k = 1.0 - ratio * ratio * (1.0 - (cosi * cosi));
	if(k < 0.0){
		refractance = 0.0;
	} else {
		vec3 refractedDir = ratio * ray.direction + (ratio * cosi - sqrt(k)) * hr.normal;
		refractedRay.origin = hr.p + 0.001 * refractedDir;
		refractedRay.direction = refractedDir;
		refractedRay.invDirection = 1.0 / refractedDir;
	}
}

void material_absorb(Ray ray, HitRecord hr, vec3 absorption, out vec3 attenuation) {
	if (!hr.frontFace) {
		float distance = distance(ray.origin, hr.p);
		attenuation.r *= exp(-absorption.x * distance);
		attenuation.g *= exp(-absorption.y * distance);
		attenuation.b *= exp(-absorption.z * distance);
	}
}

 

/*******************************************************************

LIGHT DEFINITION AND METHODS

********************************************************************/

const uint DIRECTIONAL = 0x00000001u;
const uint POINT = 0x00000002u;
const uint SPOT = 0x00000004u;
const uint AMBIENT = 0x00000008u;


struct Light {
  uint type;
  vec3 color;
  float intensity;
  vec3 position;
  float cutoffAngle;
  vec3 direction;
};

layout(std430, binding = 7) readonly buffer Lights {
	Light[] lights;
};

vec3 getIllumination(const Light light, const HitRecord rec, Ray ray) {

	vec3 illumination = vec3(0.0);
	float nd = dot(rec.normal, ray.direction);
	if(nd > 0.0){
		illumination += light.color * light.intensity * nd;
	}
	return illumination;
};

Ray getShadowRay(const Light light, const HitRecord rec, out float tMax) {
	Ray shadowRay;
	shadowRay.origin = vec3(0.0, 0.0, 0.0);
	shadowRay.direction = vec3(0.0, 0.0, 0.0);

	if (light.type == POINT) {
		tMax = distance(light.position, rec.p);
		shadowRay.direction = light.position - rec.p;
		shadowRay.origin = rec.p + 0.001 * shadowRay.direction;
	} else if (light.type == SPOT) {
		tMax = distance(light.position, rec.p);
		shadowRay.direction = light.position - rec.p;
		shadowRay.origin = rec.p + 0.001 * shadowRay.direction;

		float angle = acos(dot(shadowRay.direction, normalize(rec.p - light.position)));
		if (angle > light.cutoffAngle) {
			shadowRay.origin = rec.p;
			shadowRay.direction = vec3(0.0, 0.0, 0.0);
		}
	} else if (light.type == DIRECTIONAL) {
		tMax = INFINITY;
		shadowRay.direction = -light.direction;
		shadowRay.origin = rec.p + 0.001 * (-light.direction);
	} else if (light.type == AMBIENT) {
		tMax = 0.0001;
		shadowRay.origin = rec.p + 0.001 * rec.normal;
		shadowRay.direction = rec.normal;
	}

	shadowRay.invDirection = 1.0 / shadowRay.direction;
	return shadowRay;
}

vec3 attenuate(Light light, vec3 color, const vec3 p) {
	return color * 1.0 / distance(light.position, p);
}


/*******************************************************************

TRACING

********************************************************************/

bool traceScene(const Ray ray, float tMin, float tMax, out HitRecord rec) {
	HitRecord tmp;
	tmp.p = vec3(INFINITY, INFINITY, INFINITY);
	bool hasHit = false;
	float closest = tMax;

	for (int i = 0; i < instances.length(); i++) {
		Instance instance = instances[i];
		if (intersectBVH(ray, instance, tMin, closest, tmp)) {
			hasHit = true;
			closest = tmp.t;
			rec = tmp;
		}
	}

	return hasHit;
}

vec3 traceLights(HitRecord rec) {

	vec3 illumination = vec3(0.0);
	for (int i = 0; i < lights.length(); i++) {
		Light light = lights[i];
		float tMax;
		Ray shadowRay = getShadowRay(light, rec, tMax);

		// Required for Spotlights
		if (shadowRay.direction == vec3(0.0, 0.0, 0.0)) {
			continue;
		}

		HitRecord obstruction;
		if(!traceScene(shadowRay, EPSILON, tMax, obstruction)){
			vec3 contribution = getIllumination(light, rec, shadowRay);
			illumination += attenuate(light, contribution, rec.p);
		}
	}
	return illumination;
}

vec3 trace(Ray ray, int bounces) {

	HitRecord hr;
	hr.p = vec3(INFINITY, INFINITY, INFINITY);
	if(bounces <= 0)
		return vec3(0,0,0);

	if (traceScene(ray, 0.001, INFINITY, hr)) {
		Ray reflectedRay;
		Material mat = materials[hr.material];

		vec3 attenuation = getTextureColor(mat.albedoIdx, hr.u, hr.v, hr.p);
		float reflectance = 1.0;

		if (mat.type == DIFFUSE) {
			return attenuation * traceLights(hr);
		} else if (mat.type == MIRROR) {
			material_reflect(mat, ray, hr, reflectedRay, reflectance);
			if (reflectance == 1.0)
				return attenuation * trace(reflectedRay, bounces - 1);
			else
				return attenuation * (reflectance * trace(reflectedRay, bounces - 1) + (1.0 - reflectance) * traceLights(hr));
		} else if (mat.type == DIELECTRIC) {

			vec3 refractionColor = vec3(0.0);
			vec3 reflectionColor = vec3(0.0);
			float reflectance;

			material_reflect(mat, ray, hr, reflectedRay, reflectance);
			reflectionColor = trace(reflectedRay, bounces - 1);

			if (reflectance < 1.0) {
				Ray refractedRay;
				float refractance;
				material_refract(ray, hr, mat.ior, refractedRay, refractance);
				refractionColor = trace(refractedRay, bounces-1);
			}

			material_absorb(ray, hr, mat.absorption, attenuation);
			return attenuation * (reflectionColor * reflectance + refractionColor * (1.0 - reflectance));
		}
		return vec3(0.0, 0.0, 0.0);
	}

	return vec3(0.4, 0.4, 0.4);
}

uniform vec3 camPosition, llCorner, horizontal, vertical;
uniform int bounces;

void main() {
	Ray ray;

	ivec2 px = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebufferImage);
	if (any(greaterThanEqual(px, size)))
		return;

	ray.direction = normalize(llCorner + px.x * horizontal + px.y * vertical - camPosition);
	ray.origin = camPosition;

	vec3 color = trace(ray, bounces);
	imageStore(framebufferImage, px, vec4(color, 1.0));
}
