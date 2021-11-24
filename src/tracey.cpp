#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "materials/diffuse.hpp"
#include "materials/metal.hpp"
#include "textures/checkered.hpp"
#include "textures/image_texture.hpp"
#include "renderer.hpp"
#include "options_manager.hpp"
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
	}
}

int main(int argc, char *args[]){
	if(argc > 1){
		parseOptions(args[1]);
	}

	OptionsMap::Instance()->printOptions();
	MaterialPtr woodMat = std::make_shared<Diffuse>(std::make_shared<ImageTexture>("../Textures/parque.png"));
	MaterialPtr earthMat = std::make_shared<Diffuse>(std::make_shared<ImageTexture>("../Textures/earthmap.jpg"));
	MaterialPtr checkeredMat = std::make_shared<Diffuse>(std::make_shared<Checkered>());
	MaterialPtr redDiffuse = std::make_shared<Diffuse>(Color(1.0,0.0,0.0));
	MaterialPtr greenMetal = std::make_shared<Metal>(Color(0.0,1.0,0.0),0.4);

	Renderer renderer("TraceyGL");

	ScenePtr scene = std::make_shared<Scene>();
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.6, 0.0, -1.5}, 0.4, greenMetal));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.0, -1.0}, 0.5, earthMat));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{-1.0, 0.0, -1.0}, 0.4, redDiffuse));
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(0.0, -0.55, 0.0), glm::dvec3(0.0, 1.0, 0.0), woodMat));
	scene->addHittable(std::make_shared<Plane>(glm::dvec3(1.0, 0.0, 0.0), glm::normalize(glm::dvec3(-1.0, 0.0, 0.0)), checkeredMat));
	scene->setCamera(std::make_shared<Camera>(glm::dvec3{0.0, 0.0, 1.0}, glm::dvec3{0.0, 0.0, 0.0}, glm::dvec3{0.0, 1.0, 0.0}, 60));

	//scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.0, 0.0}, 2, earthMat));
	//scene->setCamera(std::make_shared<Camera>(glm::dvec3{13.0, 2.0, 3.0}, glm::dvec3{0.0, 0.0, 0.0}, glm::dvec3{0.0, 1.0, 0.0}, 20));
	
	renderer.setScene(scene);
	renderer.init();
	renderer.start();
}
