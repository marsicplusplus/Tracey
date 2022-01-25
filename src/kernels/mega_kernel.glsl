#version 450 core
layout (local_size_x=1, local_size_y=1) in;

struct Mesh {
	int firstTri;
	int lastTri;
	int firstNode;
	int lastNode;
};
struct Instance{
	mat4 transformMat;
	mat4 transformInv;
	mat4 transposeInv;
	int meshIdx;
};
struct Texture {
	vec4 color1;
	vec4 color2; 
	uint type; 
	int idx; 
	int width; 
	int height; 
	int slice;  
	int bpp; 
};
struct Triangle {
	vec4 v0, v1, v2; 	// 12N
	vec4 n0, n1, n2; 	// 12N
	vec4 uv0, uv1, uv2; // 12N
	int matIdx; 		// 1N
};
struct HitRecord {
	vec3 p;
	vec3 normal;
	bool frontFace;
	int material;
	float u;
	float v;
	float t;
};

struct Ray {
	vec3 origin;
	vec3 direction;
	vec3 invDirection;
};
struct Node {
	vec4 minAABB; 	// 4N
	vec4 maxAABB; 	// 4N
	uint count;		// 1N
	uint leftFirst;	// 1N
};
struct Material {
	vec4 absorption;
	uint type;
	int albedoIdx;
	int bump;
	float reflectionIdx;
	float ior;
};
struct Light {
  vec4 color;
  vec4 position;
  vec4 direction;
  uint type;
  float intensity;
  float cutoffAngle;
};



layout (binding = 0, rgba32f) uniform image2D fb;
layout(std430, binding = 1) readonly buffer Meshes {
	Mesh[] meshes;
};
layout(std430, binding = 2) readonly buffer Instances {
	Instance[] instances;
};
layout(std430, binding = 3) readonly buffer Textures {
	Texture[] textures;
};
layout(std430, binding = 4) readonly buffer Triangles {
	Triangle[] triangles;
};
layout(std430, binding = 5) readonly buffer BVHNodes {
	Node[] nodes;
};
layout(std430, binding = 6) readonly buffer Materials {
	Material[] materials;
};
layout(std430, binding = 7) readonly buffer Lights {
	Light[] lights;
};
layout(std430, binding = 8) readonly buffer Images {
	uint[] imgs;
};

const float INFINITY = 1.0/0.0;
const float EPSILON = 1e-10;
const float SCALE_BYTE = 0.00392156862745098f;
#define FLT_MIN 1.175494351e-38
const uint NILL = 0x00000000u;

/*******************************************************************

TEXTURE DEFINITION AND METHODS

********************************************************************/

const uint IMAGE = 0x00000001u;
const uint SOLID = 0x00000002u;
const uint CHECKERED = 0x00000004u;


vec4 getTextureColor(int textureIdx, float u, float v, const vec3 p) {
	Texture tex = textures[textureIdx];
	if (tex.type == SOLID) {
		return tex.color1;
	} else if (tex.type == IMAGE) {
		//int i = int(mod(u*tex.width, tex.width-1));
		//int j = int(mod(v*tex.height, tex.height-1));
		int pixel = tex.idx + int(u) * tex.slice + int(v);
		return vec4(
				((imgs[pixel] & 0x00ff0000) >> 16) * SCALE_BYTE, 
				((imgs[pixel] & 0x0000ff00) >> 8) * SCALE_BYTE, 
				((imgs[pixel] & 0x000000ff) >> 0) * SCALE_BYTE, 
				1.0f
			);
	} else if (tex.type == CHECKERED){
		float sines = sin(10.0f*p.x)*sin(10.0f*p.y)*sin(10.0f*p.z);
		return (sines < 0.0f) ? tex.color1 : tex.color2;
	}
}


/*******************************************************************

HIT RECORD AND RAY DEFINITION/METHODS

********************************************************************/

Ray transformRay(Ray origRay, mat4 transform) {
	vec3 newDir = vec3(transform * vec4(origRay.direction, 0.0));
	vec3 newOg = vec3(transform * vec4(origRay.origin, 1.0));
	Ray ret;
	ret.origin = newOg;
	ret.direction = newDir;
	ret.invDirection = 1.0/newDir;
	return ret;
}

bool hitAABB(Ray ray, vec4 minAABB, vec4 maxAABB, out float dist) {

	float tmin, tmax;

	float tx1 = (minAABB.x - ray.origin.x) * ray.invDirection.x;
	float tx2 = (maxAABB.x - ray.origin.x) * ray.invDirection.x;

	tmin = min(tx1, tx2);
	tmax = max(tx1, tx2);

	float ty1 = (minAABB.y - ray.origin.y) * ray.invDirection.y;
	float ty2 = (maxAABB.y - ray.origin.y) * ray.invDirection.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (minAABB.z - ray.origin.z) * ray.invDirection.z;
	float tz2 = (maxAABB.z - ray.origin.z) * ray.invDirection.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	dist = max(tmin, 0.0);
	return (tmax >= dist);
}

void setFaceNormal(inout HitRecord rec, const Ray r, const vec3 outNormal) {
	rec.frontFace = dot(r.direction, outNormal) < 0.0;
	rec.normal = rec.frontFace ? normalize(outNormal) : normalize(-outNormal);
}

/*******************************************************************

TRIANGLE DEFINITON AND METHODS

********************************************************************/


bool hitTriangle(Ray ray, Triangle tri, float tMin, float tMax, out HitRecord rec) {

	vec3 v0v1 = tri.v1.xyz - tri.v0.xyz;
	vec3 v0v2 = tri.v2.xyz - tri.v0.xyz;
	vec3 p = cross(ray.direction, v0v2);
	float det = dot(v0v1, p);
	if (abs(det) < EPSILON) return false;
	float inv = 1.0 / det;

	vec3 tv = ray.origin - tri.v0.xyz;
	float u = dot(tv, p) * inv;
	if (u < 0.0 || u > 1.0) return false;

	vec3 q = cross(tv, v0v1);
	float v = dot(ray.direction, q) * inv;
	if (v < 0.0 || u + v > 1.0) return false;
	float tmp = dot(v0v2, q) * inv;
	if (tmp < 0.0) return false;

	if (tmp > tMin && tmp < tMax) {

		vec3 hitNormal = vec3(u * (tri.n1) + v * (tri.n2) + (1.0 - u - v) * (tri.n0));

		setFaceNormal(rec, ray, hitNormal);

		vec2 uv = u * (tri.uv1.xy) + v * (tri.uv2.xy) + (1.0 - u - v) * (tri.uv0.xy);

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


bool traverseBVH(Ray ray, Instance instance, float tMin, float tMax, out HitRecord rec) {
	bool hasHit = false;

	int meshIdx = instance.meshIdx;

	Mesh mesh = meshes[meshIdx];
	int firstTri = mesh.firstTri;
	int firstNodeIdx = mesh.firstNode;

	Node nodestack[64];
	int stackPtr = 0;
	nodestack[stackPtr++] = nodes[firstNodeIdx]; // Push the root into the stack

	while(stackPtr != 0){
		Node currNode = nodestack[--stackPtr];

		if(currNode.count != 0) {// I'm a leaf
			for (uint i = firstTri + currNode.leftFirst; i < firstTri + currNode.leftFirst + currNode.count; ++i) {
				Triangle tri = triangles[i];
				HitRecord tmp;
				if (hitTriangle(ray, tri, tMin, tMax, tmp)) {
					rec = tmp;
					tMax = rec.t;
					hasHit = true;
				}
			}
		} else {
			uint idx1 = firstNodeIdx + currNode.leftFirst;
			uint idx2 = firstNodeIdx + currNode.leftFirst + 1;
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

const uint DIFFUSE 		= 0x00000001u;
const uint DIELECTRIC  	= 0x00000002u;
const uint MIRROR  		= 0x00000004u;


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
	reflectedRay.invDirection = 1.0 / reflectedRay.direction;
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
		refractedRay.invDirection = 1.0 / refractedRay.direction;
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


vec3 getIllumination(const Light light, const HitRecord rec, Ray ray) {

	vec4 illumination = vec4(0.0);
	float nd = dot(rec.normal, ray.direction);
	if(nd > 0.0){
		illumination += light.color * light.intensity * nd;
	}
	return illumination.xyz;
};

Ray getShadowRay(const Light light, const HitRecord rec, out float tMax) {
	Ray shadowRay;
	shadowRay.origin = vec3(0.0, 0.0, 0.0);
	shadowRay.direction = vec3(0.0, 0.0, 0.0);

	vec3 lightpos = vec3(light.position);
	vec3 lightdir = vec3(light.direction);

	if (light.type == POINT) {
		tMax = distance(lightpos, rec.p);
		shadowRay.direction = lightpos - rec.p;
		shadowRay.origin = rec.p + 0.001 * shadowRay.direction;
	} else if (light.type == SPOT) {
		tMax = distance(lightpos, rec.p);
		shadowRay.direction = lightpos - rec.p;
		shadowRay.origin = rec.p + 0.001 * shadowRay.direction;

		float angle = acos(dot(shadowRay.direction, normalize(rec.p - lightpos)));
		if (angle > light.cutoffAngle) {
			shadowRay.origin = rec.p;
			shadowRay.direction = vec3(0.0, 0.0, 0.0);
		}
	} else if (light.type == DIRECTIONAL) {
		tMax = INFINITY;
		shadowRay.direction = -lightdir;
		shadowRay.origin = rec.p + 0.001 * (-lightdir);
	} else if (light.type == AMBIENT) {
		tMax = 0.0001;
		shadowRay.origin = rec.p + 0.001 * rec.normal;
		shadowRay.direction = rec.normal;
	}

	shadowRay.invDirection = 1.0 / shadowRay.direction;
	return shadowRay;
}

vec3 attenuate(Light light, vec3 color, const vec3 p) {
	if (light.type == POINT || light.type == SPOT) {
		return color * 1.0 / distance(vec3(light.position), p);
	} else {
		return color;
	}
}


/*******************************************************************

TRACING

********************************************************************/

bool traceScene(const Ray ray, float tMin, float tMax, out HitRecord rec) {
	bool hasHit = false;

	for (int i = 0; i < instances.length(); i++) {
		HitRecord tmp;
		Instance instance = instances[i];
		if (intersectBVH(ray, instance, tMin, tMax, tmp)) {
			hasHit = true;
			tMax = tmp.t;
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
		if(!traceScene(shadowRay, 0.001, tMax, obstruction)){
			vec3 contribution = getIllumination(light, rec, shadowRay);
			illumination += attenuate(light, contribution, rec.p);
		}
	}
	return illumination;
}

vec3 trace(Ray ray, int bounces) {
	HitRecord hr;
	hr.p = vec3(INFINITY, INFINITY, INFINITY);
	vec3 c = vec3(0.0f,0.0f,0.0f);
	Ray current = ray;
	float frac = 1.0f;
	while(bounces > 0){
		if (traceScene(current, 0.001, INFINITY, hr)) {
			Ray reflectedRay;
			Material mat = materials[hr.material];
			vec3 attenuation = getTextureColor(mat.albedoIdx, hr.u, hr.v, hr.p).xyz;
			float reflectance = 1.0f;
			if (mat.type == DIFFUSE || mat.type == DIELECTRIC) {
				c += attenuation * traceLights(hr) * frac;
				break;
			} else if(mat.type == MIRROR) {
				material_reflect(mat, current, hr, reflectedRay, reflectance);
				current = reflectedRay;
				bounces--;

				c += attenuation * traceLights(hr) * (1.0f - reflectance) * frac;
				frac *= reflectance;
				if(reflectance == 0.0f) break;
				continue;
			} else {
				return vec3(0.4f);
				break;
			}
		}else{
			break;
		}
	}
	return c;
}

uniform vec3 camPosition, llCorner, horizontal, vertical;
uniform int bounces;

void main() {
	Ray ray;

	vec2 px = gl_GlobalInvocationID.xy;
	vec2 size = gl_NumWorkGroups.xy;

	ray.direction = normalize(llCorner + px.x / (size.x - 1.0f) * horizontal + px.y / (size.y - 1.0f) * vertical - camPosition);
	ray.origin = camPosition;
	ray.invDirection = 1.0 / ray.direction;

	//float t = 0.5 * (ray.direction.y + 1.0);
	//vec3 color = (1.0 - t) * vec3(1.0, 0.0, 0.0) + t * vec3(0.0, 0.0, 0.0);
	
	vec3 color = trace(ray, bounces);

	imageStore(fb, ivec2(gl_GlobalInvocationID.xy), vec4(color.rgb, 1.0));
}
