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

void parseOptions(std::filesystem::path path){
	std::ifstream config(path);
	std::string line;
	while(std::getline(config, line)){
		if(line[0] == '#') continue;
		std::string key = line.substr(0, line.find('='));
		line.erase(0, line.find('=') + 1);
		if(key == "MAX_BOUNCES") OptionsMap::Instance()->setOption(Options::MAX_BOUNCES, std::stoi(line));
		if(key == "FPS_LIMIT") OptionsMap::Instance()->setOption(Options::FPS_LIMIT, std::stoi(line));
		if(key == "SAMPLES") OptionsMap::Instance()->setOption(Options::SAMPLES, std::stoi(line));
		if(key == "TILE_WIDTH") OptionsMap::Instance()->setOption(Options::TILE_WIDTH, std::stoi(line));
		if(key == "TILE_HEIGHT") OptionsMap::Instance()->setOption(Options::TILE_HEIGHT, std::stoi(line));
		if(key == "W_HEIGHT") OptionsMap::Instance()->setOption(Options::W_HEIGHT, std::stoi(line));
		if(key == "W_WIDTH") OptionsMap::Instance()->setOption(Options::W_WIDTH, std::stoi(line));
		if(key == "SCALING") OptionsMap::Instance()->setOption(Options::SCALING, std::stoi(line));
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
	std::string scenePath = "";
	std::string configPath = "";
	for(int i = 1; i < argc; i++){
		if(strncmp("config=", args[i], strlen("config=")) == 0){
			configPath = (&args[i][strlen("config=")]);
		}
		if(strncmp("scene=", args[i], strlen("scene=")) == 0){
			scenePath = (&args[i][strlen("scene=")]);
		}
	}
	stbi_set_flip_vertically_on_load(true);


	ScenePtr scene;
	if(!configPath.empty() && std::filesystem::exists(configPath)){
		parseOptions(configPath.c_str());
	}

	OptionsMap::Instance()->printOptions();
	Threading::pool.init(OptionsMap::Instance()->getOption(Options::THREADS));

	try{
		if(!scenePath.empty() && std::filesystem::exists(scenePath)){
			scene = std::make_shared<Scene>(scenePath);
		}
	} catch(std::exception &e){
		std::cout << "Failed parsing the scene file: " << e.what() << std::endl;
	}


	Renderer renderer("TraceyGL");
	if(scene) renderer.setScene(scene);
	renderer.init();
	renderer.start();
	Threading::pool.cancel_pending();
}
