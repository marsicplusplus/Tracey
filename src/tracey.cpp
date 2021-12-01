#define STB_IMAGE_IMPLEMENTATION
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
				nThreads = (nThreads < 1) ? hardwareConcurrency : (std::min)(hardwareConcurrency, nThreads);
			}else{
				nThreads = 1;
			}
			OptionsMap::Instance()->setOption(Options::THREADS, nThreads);
		}
	}
}

void printHelp(char *name){
	printf("USAGE:\n%s [scene=<path-to-scene.json>] [config=<path-to-config.txt>]\n", name);
}

int main(int argc, char *args[]){
	ScenePtr scene;
	for(int i = 1; i < argc; i++){
		if(strncmp("config=", args[i], strlen("config=")) == 0){
			parseOptions(&args[i][strlen("config=")]);
		}
		if(strncmp("scene=", args[i], strlen("scene=")) == 0){
			scene = std::make_shared<Scene>(&args[i][strlen("scene=")]);
		}
	}
	OptionsMap::Instance()->printOptions();

	Renderer renderer("TraceyGL", OptionsMap::Instance()->getOption(Options::THREADS));
	if(scene) renderer.setScene(scene);
	renderer.init();
	renderer.start();
}
