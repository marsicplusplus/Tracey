#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "materials/material_diffuse.hpp"
#include "materials/material_dielectric.hpp"
#include "materials/material_mirror.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "light_object.hpp"
#include "renderer.hpp"
#include "options_manager.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <memory>

void parseOptions(char *path){
	std::ifstream config(path);
	std::string line;
	while(std::getline(config, line)){
		std::string key = line.substr(0, line.find('='));
		line.erase(0, line.find('=') + 1);
		if(key == "MAX_BOUNCES") OptionsMap::Instance()->setOption(Options::MAX_BOUNCES, std::stoi(line));
		if(key == "FPS_LIMIT") OptionsMap::Instance()->setOption(Options::FPS_LIMIT, std::stoi(line));
		if(key == "SAMPLES") OptionsMap::Instance()->setOption(Options::SAMPLES, std::stoi(line));
		if(key == "TILE_WIDTH") OptionsMap::Instance()->setOption(Options::TILE_WIDTH, std::stoi(line));
		if(key == "TILE_HEIGHT") OptionsMap::Instance()->setOption(Options::TILE_HEIGHT, std::stoi(line));
		if(key == "W_HEIGHT") OptionsMap::Instance()->setOption(Options::W_HEIGHT, std::stoi(line));
		if(key == "W_WIDTH") OptionsMap::Instance()->setOption(Options::W_WIDTH, std::stoi(line));
		if(key == "THREADS"){
			int nThreads = std::stoi(line);
			// std::thread::hardware_concurrency() can return 0 on failure 
			int hardwareConcurrency = std::thread::hardware_concurrency();
			if(hardwareConcurrency != 0){
				nThreads = (nThreads < 1) ? hardwareConcurrency : std::min(hardwareConcurrency, nThreads);
			}else{
				nThreads = 1;
			}
			OptionsMap::Instance()->setOption(Options::THREADS, nThreads);
		}
	}
}

int main(int argc, char *args[]){
	if(argc > 1){
		parseOptions(args[1]);
	}

	OptionsMap::Instance()->printOptions();
	auto wood = std::make_shared<ImageTexture>("Textures/parque.png");
	auto checkered = std::make_shared<Checkered>();
	/* Materials */
	MaterialPtr woodMat = std::make_shared<MirrorMaterial>(checkered, 0.5);
	MaterialPtr earthMat = std::make_shared<DiffuseMaterial>(std::make_shared<ImageTexture>("Textures/earthmap.jpg"));
	MaterialPtr checkeredMat = std::make_shared<DiffuseMaterial>(checkered);
	MaterialPtr redDiffuse = std::make_shared<DiffuseMaterial>(Color(1.0,0.0,0.0));
	MaterialPtr greenDiffuse = std::make_shared<DiffuseMaterial>(Color(0.0,1.0,0.0));
	MaterialPtr blueDiffuse = std::make_shared<DiffuseMaterial>(Color(0.0,0.0,1.0));
	MaterialPtr greenMirror = std::make_shared<MirrorMaterial>(Color(0.0,1.0,0.0), /* reflective */ 1.0);
	MaterialPtr whiteMirror = std::make_shared<MirrorMaterial>(Color(1.0), /* reflective */ 1.0);
	MaterialPtr glass = std::make_shared<DielectricMaterial>(Color(1.0,1.0,1.0), /* ref_idx */ 1.5);
	MaterialPtr water = std::make_shared<DielectricMaterial>(Color(0.0,0.2,0.8), /* ref_idx */ 1.3);

	ScenePtr scene = std::make_shared<Scene>();
	/* Hittables */
#if 1
	scene->addHittable(std::make_shared<ZXRect>(-0.5, glm::dvec4(-2.0, 2.0, -2.0, 2.0), woodMat));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.2, 0.0}, 0.5, glass));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{-0.1, 0.4, -1.0}, 0.4, earthMat));
#else 
	 //// //Spheres
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.8, 0.0, -4.6}, 0.4, earthMat));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, -0.2, -1.5}, 0.3, redDiffuse));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{-1.0, 0.0, -0.6}, 0.4, greenMirror));

	 //// //Planes
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(0.0, -0.5, 0.0), glm::normalize(glm::dvec3(0.0, 1.0, 0.0)), woodMat));
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(4.0, 0.0, 0.0), glm::normalize(glm::dvec3(-1.0, 0.0, 0.0)), checkeredMat));
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(-2.0, 0.0, 0.0), glm::normalize(glm::dvec3(1.0, 0.0, 0.0)), checkeredMat));
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(0.0, 0.0, -2.5), glm::normalize(glm::dvec3(0.0, 0.0, 1.0)), whiteMirror));

#endif
	 /* Lights */
	//scene->addLight(std::make_shared<AmbientLight>(1.0, Color{1.0, 1.0, 1.0}));
	scene->addLight(std::make_shared<PointLight>(glm::dvec3(1.5, 2.0, -1.5), 1.4, glm::dvec3(1,0.5,1)));
	scene->addLight(std::make_shared<PointLight>(glm::dvec3(-1.5, 2.0, -1.5), 1.4, glm::dvec3(1,1,1)));
	scene->addLight(std::make_shared<DirectionalLight>(glm::dvec3(0.0, -1.0, 0.0), 1.2, glm::dvec3(0.8,0.8,0.6)));

	/* Camera */
	scene->setCamera(std::make_shared<Camera>(glm::dvec3{0.0, 0.0, 1.0}, glm::dvec3{0.0, 0.0, -1.0}, glm::dvec3{0.0, 1.0, 0.0}, 60));

	Renderer renderer("TraceyGL", OptionsMap::Instance()->getOption(Options::THREADS));
	renderer.setScene(scene);
	renderer.init();
	renderer.start();
}
