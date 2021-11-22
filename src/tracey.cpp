#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "materials/diffuse.hpp"
#include "materials/metal.hpp"
#include "textures/checkered.hpp"
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

	OptionsMap::Instance()->printOptions();

	Renderer renderer("TraceyGL");

	ScenePtr scene = std::make_shared<Scene>();
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.6, 0.0, -1.5}, 0.4, std::make_shared<Metal>(Color(1.0, 1.0, 0.6), 0.2)));
	scene->addHittable(std::make_shared<Sphere>(glm::dvec3{0.0, 0.0, -1.0}, 0.5, std::make_shared<Diffuse>(Color(1.0, 0.0, 0.0))));
	scene->addHittable(std::make_shared<ZXPlane>(-0.5, glm::dvec4(-10.0, 10.0, -10.0, 10.0), std::make_shared<Diffuse>(std::make_shared<Checkered>())));
	renderer.setScene(scene);
	
	renderer.init();
	renderer.start();
}
