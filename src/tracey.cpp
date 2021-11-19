#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
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
	}
}

int main(int argc, char *args[]){
	if(argc > 1){
		parseOptions(args[1]);
	}

	Renderer renderer("TraceyGL");

	ScenePtr scene = std::make_shared<Scene>();
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.0, -1.0}, 0.4));
	//scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.7, -1.0}, 0.2));
	//scene->addHittable(std::make_shared<Plane>(glm::dvec3{0.0, -0.5, 0.0}, glm::dvec3{0.0, -1.0, 0.0}));
	renderer.setScene(scene);
	
	renderer.init();
	renderer.start();
}
