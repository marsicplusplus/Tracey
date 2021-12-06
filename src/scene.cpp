#include "scene.hpp"

#include "GLFW/glfw3.h"
#include "input_manager.hpp"
#include "json.hpp"
#include "textures/texture.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "hittables/mesh.hpp"
#include "hittables/torus.hpp"
#include "materials/material.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "materials/material_diffuse.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>
#include <fstream>

static glm::dvec3 parseVec3(nlohmann::basic_json<> &arr){
	return glm::dvec3(arr[0].get<double>(), arr[1].get<double>(),arr[2].get<double>());
}

static glm::dvec4 parseVec4(nlohmann::basic_json<> &arr){
	return glm::dvec4(arr[0].get<double>(), arr[1].get<double>(),arr[2].get<double>(), arr[3].get<double>());
}

std::shared_ptr<Hittable> Scene::parseMesh(std::filesystem::path &path, std::shared_ptr<Material> mat) const {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate = true;
	reader_config.mtl_search_path = path.parent_path().string(); // Path to material files

	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path.string(), reader_config)) {
		if (!reader.Error().empty()) std::cerr << "TinyObjReader: " << reader.Error();
		return nullptr;
	}
	if (!reader.Warning().empty()) std::cout << "TinyObjReader: " << reader.Warning();

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	std::vector<glm::dvec3> pos;
	std::vector<glm::dvec3> norm;
	std::vector<glm::dvec2> uv;
	std::vector<Triangle> triangles;

	for(size_t i = 0; i < attrib.vertices.size(); i+=3) {
		pos.push_back(glm::dvec3{
				attrib.vertices[i],
				attrib.vertices[i+1],
				attrib.vertices[i+2],
				});
	}
	for(size_t i = 0; i < attrib.normals.size(); i+=3) {
		norm.push_back(glm::dvec3{
				attrib.normals[i],
				attrib.normals[i+1],
				attrib.normals[i+2],
				});
	}
	for(size_t i = 0; i < attrib.texcoords.size(); i+=2) {
		uv.push_back(glm::dvec2{
				attrib.texcoords[i],
				attrib.texcoords[i+1],
				});
	}

	for(auto &shape : shapes){
		const std::vector<tinyobj::index_t> idx = shape.mesh.indices;
		const std::vector<int> & mat_ids = shape.mesh.material_ids;
		for(size_t face_ind = 0; face_ind < mat_ids.size(); face_ind++) {
			triangles.push_back(Triangle(
						glm::ivec3{idx[3*face_ind].vertex_index, 	idx[3*face_ind+1].vertex_index, 	idx[3*face_ind+2].vertex_index},
						glm::ivec3{idx[3*face_ind].normal_index, 	idx[3*face_ind+1].normal_index, 	idx[3*face_ind+2].normal_index},
						glm::ivec3{idx[3*face_ind].texcoord_index, 	idx[3*face_ind+1].texcoord_index, 	idx[3*face_ind+2].texcoord_index},
						mat
					));
		}
	}
	std::cout << "Vertices: " << std::endl;
	int i=0;
	for(auto &v : pos){
		std::cout << i++ << ": (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
	}
	i=0;
	std::cout << "Normals: " << std::endl;
	for(auto &v : norm){
		std::cout << i++ << ": (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
	}
	std::cout << "Textures: " << std::endl;
	for(auto &v : uv){
		std::cout << i++ << ": (" << v.x << ", " << v.y << ")" << std::endl;
	}
	std::cout << "Indices: " << std::endl;
	i=0;
	for(auto &v : triangles){
		std::cout << i << ": (" << v.face.x << ", " << v.face.y << ", " << v.face.z << ")" << std::endl;
		std::cout << i << ": (" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z << ")" << std::endl;
		std::cout << i++ << ": (" << v.texture.x << ", " << v.texture.y << ", " << v.texture.z << ")" << std::endl;
	}
	return std::make_shared<Mesh>(pos, norm, uv, triangles);
}

Scene::Scene(){}
Scene::Scene(std::filesystem::path sceneFile){
	std::filesystem::path currPath = std::filesystem::current_path();
	std::filesystem::current_path(sceneFile.parent_path());
	std::ifstream i(sceneFile.filename());
	auto j = nlohmann::json::parse(i);

	auto camera = parseCamera(j["camera"]);
	setCamera(camera);

	for(auto t : j["textures"]){
		auto text = parseTexture(t);
		if (text.second){
			textures.insert(text);
		}
	}
	for(auto m : j["materials"]){
		auto mat = parseMaterial(m);
		if (mat.second){
			materials.insert(mat);
		}
	}
	for(auto p : j["primitives"]){
		auto hit=parseHittable(p);
		if(hit)
			hittables.push_back(hit);
	}
	for(auto l : j["lights"]){
		auto light=parseLight(l);
		if(light)
			lights.push_back(light);
	}
	std::filesystem::current_path(currPath);
}
Scene::~Scene(){}

void Scene::addHittable(HittablePtr hittable){
	this->hittables.push_back(hittable);
}

void Scene::addLight(LightObjectPtr light){
	this->lights.push_back(light);
}

bool Scene::traverse(const Ray &ray, double tMin, double tMax, HitRecord &rec) const {
	HitRecord tmp;
	bool hasHit = false;
	double closest = tMax;

	for (const auto& object : this->hittables) {
		if (object->hit(ray, tMin, closest, tmp)) {
			hasHit = true;
			closest = tmp.t;
			rec = tmp;
		}
	}
	return hasHit;
}

void Scene::setCamera(CameraPtr camera){
	this->currentCamera = camera;
}

CameraPtr Scene::getCamera() const {
	return this->currentCamera;
}

bool Scene::update(double dt){
	return this->currentCamera->update(dt);
}

Color Scene::traceLights(HitRecord &rec) const {
	Color illumination(0.0);
	for(auto &light : lights){
		double tMax;
		Ray shadowRay = light->getRay(rec, tMax);

		// Required for Spotlights
		if (shadowRay.getDirection() == glm::dvec3(0, 0, 0)) {
			continue;
		}

		HitRecord obstruction;
		if(!traverse(shadowRay, 0.0001, tMax, obstruction)){
			auto contribution = light->getLight(rec, shadowRay);
			illumination += light->attenuate(contribution, rec.p);
		}
	}
	return illumination;
}

CameraPtr Scene::parseCamera(nlohmann::json &cam) const {
	glm::dvec3 pos = parseVec3(cam.at("position"));
	glm::dvec3 dir = parseVec3(cam.at("dir"));
	glm::dvec3 up = parseVec3(cam.at("up"));
	double fov = (cam.at("fov"));
	return std::make_shared<Camera>(pos, dir, up, fov);	
}

std::shared_ptr<Hittable> Scene::parseHittable(nlohmann::json &hit) const {
	if(!hit.contains("type")) throw std::invalid_argument("Hittable doesn't name a type");
	std::string type = hit.at("type");
	if(!hit.contains("material")) throw std::invalid_argument("Hittable doesn't name a material");
	auto material = materials.find(hit.at("material"));
	if(material == materials.end()) throw std::invalid_argument("Hittable doesn't name a valid material");

	if(type == "PLANE"){
		glm::dvec3 pos(parseVec3(hit["position"]));
		if(!hit.contains("normal")) throw std::invalid_argument("Plane doesn't specify a normal");
		glm::dvec3 norm(parseVec3(hit.at("normal")));
		return (std::make_shared<Plane>(pos, norm, material->second));

	} else if(type == "SPHERE"){
		glm::dvec3 pos(parseVec3(hit["position"]));
		double radius = (hit.contains("radius")) ? hit["radius"].get<double>() : 0.5;
		return (std::make_shared<Sphere>(pos, radius, material->second));

	} else if (type == "TORUS"){
		if (!hit.contains("material")) throw std::invalid_argument("Hittable doesn't name a material");
		auto material = materials.find(hit.at("material"));
		if (material == materials.end()) throw std::invalid_argument("Hittable doesn't name a valid material");

		glm::dvec3 pos(parseVec3(hit["position"]));

		double xRot = hit.contains("xRot") ? hit["xRot"].get<double>() : 0.0;
		double yRot = hit.contains("yRot") ? hit["yRot"].get<double>() : 0.0;
		double zRot = hit.contains("zRot") ? hit["zRot"].get<double>() : 0.0;

		auto torusTransform(glm::dmat4x4(1));
		torusTransform = glm::translate(torusTransform, pos);
		torusTransform = glm::rotate(torusTransform, glm::radians(xRot), glm::dvec3(1, 0, 0));
		torusTransform = glm::rotate(torusTransform, glm::radians(yRot), glm::dvec3(0, 1, 0));
		torusTransform = glm::rotate(torusTransform, glm::radians(zRot), glm::dvec3(0, 0, 1));
		double radiusMajor = (hit.contains("radiusMajor")) ? hit["radiusMajor"].get<double>() : 0.5;
		double radiusMinor = (hit.contains("radiusMinor")) ? hit["radiusMinor"].get<double>() : 0.1;
		return (std::make_shared<Torus>(radiusMajor, radiusMinor, torusTransform, material->second));

	} else if(type == "ZXRect"){
		if(!hit.contains("size")) throw std::invalid_argument("ZXRect doesn't specify a correct size [x0, x1, z0, z1]");
		glm::dvec4 size(parseVec4(hit.at("size")));
		if(!hit.contains("y")) throw std::invalid_argument("ZXRect doesn't specify a correct y");
		double yCoord(hit.at("y"));
		return std::make_shared<ZXRect>(yCoord, size, material->second);

	} else if(type == "MESH"){
		if(!hit.contains("path")) throw std::invalid_argument("Mesh doesn't specify a valid path;");
		std::filesystem::path p = hit.at("path");
		return parseMesh(p, material->second);
	} else{
		throw std::invalid_argument("Hittable doesn't name a valid type");
	}
}
std::shared_ptr<LightObject> Scene::parseLight(nlohmann::json &l) const{
	if(!l.contains("type")) throw std::invalid_argument("LightObject doesn't name a type");
	std::string type = l.at("type");
	Color color = (l.contains("color")) ? (parseVec3(l["color"])) : Color(1.0);
	double intensity = (l.contains("intensity")) ? (double)(l.at("intensity")) : 1.0;
	if(type == "POINT"){
		glm::dvec3 pos(parseVec3(l["position"]));
		return (std::make_shared<PointLight>(pos, intensity, color));
	} else if(type == "DIRECTIONAL"){
		glm::dvec3 dir(parseVec3(l["direction"]));
		return (std::make_shared<DirectionalLight>(dir, intensity, color));
	} else if(type == "AMBIENT"){
		return (std::make_shared<AmbientLight>(intensity, color));
	} else if (type == "SPOT") {
		glm::dvec3 pos(parseVec3(l["position"]));
		glm::dvec3 dir(parseVec3(l["direction"]));
		double cutoff = l.contains("cutoffAngle") ? (double)l.at("cutoffAngle") : 45.0;
		return (std::make_shared<SpotLight>(pos, dir, glm::radians(cutoff), intensity, color));
	} else {
		throw std::invalid_argument("LightObject doesn't name a valid type");
	}
}

std::pair<std::string, std::shared_ptr<Material>> Scene::parseMaterial(nlohmann::json &m) const {
	std::pair<std::string, std::shared_ptr<Material>> mat;
	if(!m.contains("name")) throw std::invalid_argument("Material is missing name");
	mat.first = m.at("name");
	if(!m.contains("type")) throw std::invalid_argument("Material is missing type");
	std::string type = m.at("type");
	if(!m.contains("texture")) throw std::invalid_argument("Material is missing texture");
	std::string textname = m.at("texture");
	auto texture = textures.find(textname);
	if(texture == textures.end()) throw std::invalid_argument("Material doesn't name a valid texture");
	if(type == "DIFFUSE"){
		mat.second = std::make_shared<DiffuseMaterial>(texture->second);
	} else if(type == "MIRROR") {
		double ref = m.contains("reflect") ? m["reflect"].get<double>() : 1.0;
		mat.second = std::make_shared<MirrorMaterial>(texture->second, ref);
	}
	else if (type == "DIELECTRIC") {
		double ior = (m.contains("ior")) ? (double)m.at("ior") : 1.0;
		Color absorption = m.contains("absorption") ? parseVec3(m.at("absorption")) : Color(1.0,1.0,1.0);
		mat.second = std::make_shared<DielectricMaterial>(texture->second, ior, absorption);
	}
	return mat;
}

std::pair<std::string, std::shared_ptr<Texture>> Scene::parseTexture(nlohmann::json &text) const {
	std::pair<std::string, std::shared_ptr<Texture>> texture;
	std::string type;
	try{
		texture.first = text.at("name");
		 type = text.at("type");
	} catch (std::exception &e){
		throw std::invalid_argument("Cannot find name or type of texture");
	}
	if(type == "SOLID_COLOR"){
		texture.second = std::make_shared<SolidColor>((text.contains("color")) ? parseVec3(text["color"]) : Color(0,0,0));
	} else if(type == "CHECKERED"){
		if(text.contains("color1") && text.contains("color2")){
			Color c1 = parseVec3(text["color1"]);
			Color c2 = parseVec3(text["color2"]);
			texture.second = std::make_shared<Checkered>(c1,c2);
		} else {
			texture.second = std::make_shared<Checkered>();
		}
	} else if(type == "IMAGE"){
		if (text.contains("path"))
			texture.second = std::make_shared<ImageTexture>(text["path"].get<std::string>());
		else 
			throw std::invalid_argument("Image Texture doesn't contains a valid path");
	}
	return texture;
}
