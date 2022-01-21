#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "scene_parser.hpp"
#include "glm/trigonometric.hpp"
#include "textures/image_texture.hpp"
#include "textures/solid_color.hpp"
#include "textures/checkered.hpp"

#include <fstream>

Scene::Scene(){}
Scene::Scene(std::filesystem::path sceneFile){
	std::filesystem::path currPath = std::filesystem::current_path();
	std::filesystem::current_path(sceneFile.parent_path());
	std::ifstream i(sceneFile.filename());
	auto j = nlohmann::json::parse(i);

	auto camera = SceneParser::parseCamera(j["camera"]);
	setCamera(camera);

	for(auto t : j["textures"]){
		auto text = SceneParser::parseTexture(t);
		if (text){
			textures.push_back(std::move(text));
		}
	}
	for(auto m : j["materials"]){
		auto mat = SceneParser::parseMaterial(m, textures);
		if (mat){
			materials.push_back(mat);
		}
	}

	if(j.contains("instance_meshes")){
		for (auto m : j["instance_meshes"]) {
			std::string name;
			std::string materialName;
			auto mesh = SceneParser::parseMeshInstance(m, materials, textures, meshes, name);
			if (mesh){
				meshesBVH[name] = mesh;
			}
		}
	}

	for(auto l : j["lights"]){
		auto light = SceneParser::parseLight(l);
		if(light)
			lights.push_back(light);
	}
	topLevelBVH = SceneParser::parseSceneGraph(j["scenegraph"], materials, meshesBVH, BVHs, nTris);
	std::cout << "Total Number of triangles: " << nTris << std::endl;
	std::filesystem::current_path(currPath);
}

Scene::~Scene(){}

void Scene::addLight(std::shared_ptr<LightObject> light){
	this->lights.push_back(light);
}

bool Scene::traverse(const Ray &ray, float tMin, float tMax, HitRecord &rec) const {
	HitRecord tmp;
	tmp.p = {INF, INF, INF};
	bool hasHit = false;
	float closest = tMax;

	if (topLevelBVH->hit(ray, tMin, closest, tmp)) {
		hasHit = true;
		closest = tmp.t;
		rec = tmp;
	}

	return hasHit;
}

void Scene::packetTraverse(std::vector<Ray>& corners, std::vector<RayInfo>& packet, float tMin) const {
	Frustum frustum;
	auto O = corners[0].getOrigin();
	frustum.normals[0] = glm::cross(corners[0].getDirection() - O, corners[1].getDirection() - corners[0].getDirection()); 
	frustum.normals[1] = glm::cross(corners[1].getDirection() - O, corners[2].getDirection() - corners[1].getDirection()); 
	frustum.normals[2] = glm::cross(corners[2].getDirection() - O, corners[3].getDirection() - corners[2].getDirection()); 
	frustum.normals[3] = glm::cross(corners[3].getDirection() - O, corners[0].getDirection() - corners[3].getDirection()); 
	frustum.offsets[0] = glm::dot(frustum.normals[0], O);
	frustum.offsets[1] = glm::dot(frustum.normals[1], O);
	frustum.offsets[2] = glm::dot(frustum.normals[2], O);
	frustum.offsets[3] = glm::dot(frustum.normals[3], O);
	topLevelBVH->packetHit(packet, frustum, tMin, 0, 0);
}


void Scene::setCamera(CameraPtr camera){
	this->currentCamera = camera;
}

const CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(float dt){
	bool ret = false;
	for(auto &m : meshesBVH){
		if(m.second->update(dt)){
			auto instanceList = BVHs[m.first];
			for(auto &i : instanceList){
				i->constructSubBVH();
			}
			ret = true;
		}
	}
	for(auto &b : BVHs){
		auto instanceList = b.second;
		for(auto &i : instanceList){
			ret|=i->update(dt);
		}
	}
	if(ret) topLevelBVH->constructTopLevelBVH();
	return ret | this->currentCamera->update(dt);
}

Color Scene::traceLights(HitRecord &rec) const {
	Color illumination(0.0f);
	for(auto &light : lights){
		float tMax;
		Ray shadowRay = light->getRay(rec, tMax);

		// Required for Spotlights
		if (shadowRay.getDirection() == glm::fvec3(0, 0, 0)) {
			continue;
		}

		HitRecord obstruction;
		if(!traverse(shadowRay, EPS, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}

void Scene::getTextureBuffer(std::vector<CompactTexture> &compactTextures, std::vector<unsigned char> &imgs) {
	for(const auto &text : textures) {
		CompactTexture txt;
		txt.type = (unsigned int) text->getType();
		if(text->getType() == TextureType::TEXTURE_SOLID){
			auto imgT = std::static_pointer_cast<SolidColor>(text);
			txt.color = imgT->c;
		} else if(text->getType() == TextureType::TEXTURE_CHECKERED){
			auto imgT = std::static_pointer_cast<Checkered>(text);
			txt.color = imgT->c1;
			txt.sec_color = imgT->c2;
		} else if(text->getType() == TextureType::TEXTURE_IMAGE){
			auto imgT = std::static_pointer_cast<ImageTexture>(text);
			txt.bpp = imgT->bpp;
			txt.w = imgT->width;
			txt.h = imgT->height;
			auto img = imgT->img;
			txt.idx = imgs.size(); // from imgs.size to imgs.size + txt.w * txt.h * txt.bpp;
			for(int i = 0; i < txt.w * txt.h * txt.bpp; ++i){
				imgs.push_back(img[i]);
			}
		}
	}
}

void Scene::getMeshBuffer(std::vector<CompactTriangle> &ctris, std::vector<BVHNode> &bvhs, std::vector<CompactMesh> &meshes){
	for(const auto &m : this->meshesBVH){
		auto tris = m.second->getHittable();
		CompactMesh cmesh;
		cmesh.firstTri = ctris.size();
		cmesh.lastTri = ctris.size() + tris.size() - 1;
		for(auto t : tris){
			auto tri = std::static_pointer_cast<Triangle>(t);
			CompactTriangle ct;
			tri->copyTri(&ct);
			ctris.push_back(ct);
		}
		int size;
		auto nodes = m.second->getNodes(size);
		cmesh.firstNode = bvhs.size();
		cmesh.lastNode = bvhs.size() + size - 1;
		for(int i=0; i < size; ++i){
			BVHNode node;
			node.maxAABBCount = nodes[i].maxAABBCount;
			node.minAABBLeftFirst = nodes[i].minAABBLeftFirst;
		}
	}
}
