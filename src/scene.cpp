#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "scene_parser.hpp"
#include "glm/trigonometric.hpp"
#include "textures/image_texture.hpp"
#include "textures/solid_color.hpp"
#include "textures/checkered.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"

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

void Scene::getTextureBuffer(std::vector<CompactTexture> &compactTextures, std::vector<unsigned int> &imgs) {
	for(const auto &text : textures) {
		CompactTexture txt;
		txt.type = toUnderlyingType(text->getType());
		if(text->getType() == TextureType::TEXTURE_SOLID){
			auto imgT = std::static_pointer_cast<SolidColor>(text);
			txt.color = glm::vec4(imgT->c, 0);
		} else if(text->getType() == TextureType::TEXTURE_CHECKERED){
			auto imgT = std::static_pointer_cast<Checkered>(text);
			txt.color = glm::vec4(imgT->c1, 0);
			txt.sec_color = glm::vec4(imgT->c2, 0);
		} else if(text->getType() == TextureType::TEXTURE_IMAGE){
			auto imgT = std::static_pointer_cast<ImageTexture>(text);
			txt.bpp = imgT->bpp;
			txt.slice = imgT->width;
			txt.w = imgT->width;
			txt.h = imgT->height;
			auto img = imgT->img;
			txt.idx = imgs.size(); // from imgs.size to imgs.size + txt.w * txt.h * txt.bpp;
			for(int i = 0; i < txt.w * txt.h * txt.bpp; i+=txt.bpp){
				unsigned int color = img[i] << 16 | img[i+1] << 8 | img[i+2] << 0; 
				imgs.push_back(color);
			}
		}
		compactTextures.push_back(txt);
	}
}

void Scene::getMeshBuffer(std::vector<CompactTriangle> &ctris, std::vector<BVHNode> &bvhs, std::vector<CompactMesh> &meshes){
	for(const auto &m : this->meshesBVH){
		auto tris = m.second->getHittables();
		auto trisIdxs = m.second->getHittableIdxs();
		CompactMesh cmesh;
		cmesh.firstTri = ctris.size();
		cmesh.lastTri = ctris.size() + tris.size() - 1;
		for(auto idx : trisIdxs){
			auto tri = std::static_pointer_cast<Triangle>(tris[idx]);
			CompactTriangle ct;
			tri->copyTri(&ct);
			ctris.push_back(ct);
		}
		int size;
		auto nodes = m.second->getNodes(size);
		cmesh.firstNode = bvhs.size();
		cmesh.lastNode = bvhs.size() + size - 1;
		meshes.push_back(cmesh);

		for(int i=0; i < size; ++i){
			BVHNode node;
			node.minAABB = nodes[i].minAABB;
			node.maxAABB = nodes[i].maxAABB;
			node.leftFirst = nodes[i].leftFirst;
			node.count = nodes[i].count;
			bvhs.push_back(node);
		}
	}
}

void Scene::getInstanceBuffer(std::vector<Instance>& instances) {
	std::vector<std::string> meshNames;
	auto index = 0;
	for(const auto &m : this->meshesBVH){
		for(const auto& bvh : this->BVHs[m.first]){
			auto transform = bvh->getTransform();
			Instance instance;
			instance.meshIdx = index;
			instance.transformMat = transform.getMatrix();
			instance.transformInv = transform.getInverse();
			instance.transposeInv = transform.getTransposeInverse();
			instances.push_back(instance);
		}
		index++;
	}
}

void Scene::getLightBuffer(std::vector<CompactLight>& compLights) {

	for (auto& light : lights) {
		CompactLight compLight;
		compLight.type = toUnderlyingType(light->getType());
		compLight.color = glm::vec4(light->getColor(), 0);
		compLight.intensity = light->getIntensity();

		if (light->getType() == Lights::DIRECTIONAL) {
			auto dirLight = std::static_pointer_cast<DirectionalLight>(light);
			compLight.direction = glm::vec4(dirLight->getDirection(), 0);

		} else if (light->getType() == Lights::SPOT) {
			auto spotLight = std::static_pointer_cast<SpotLight>(light);
			compLight.cutoffAngle = spotLight->getCutoffAngle();
			compLight.position = glm::vec4(spotLight->getPosition(), 0);
			compLight.direction = glm::vec4(spotLight->getDirection(), 0);
		} else if (light->getType() == Lights::POINT) {
			auto spotLight = std::static_pointer_cast<PointLight>(light);
			compLight.position = glm::vec4(spotLight->getPosition(), 0);

		}
		compLights.push_back(compLight);
	}
}

void Scene::getMaterialBuffer(std::vector<CompactMaterial>& compMaterials) {
	for (auto& mat : materials) {
		CompactMaterial compMat;
		compMat.type = toUnderlyingType(mat->getType());
		compMat.bump = mat->getBump();
		compMat.albedoIdx = mat->getAlbedoIdx();

		if (mat->getType() == Materials::MIRROR) {
			auto mirMat = std::static_pointer_cast<MirrorMaterial>(mat);
			compMat.reflectionIdx = mirMat->getReflectionIdx();

		} else if (mat->getType() == Materials::DIELECTRIC) {
			auto dielMat = std::static_pointer_cast<DielectricMaterial>(mat);
			compMat.absorption = glm::vec4(dielMat->getAbsorption(), 0);
			compMat.ior = dielMat->getIOR();
		}
		compMaterials.push_back(compMat);
	}
}
