#ifndef __OPTIONS_MANAGER_HPP__
#define __OPTIONS_MANAGER_HPP__

#include "defs.hpp"
#include <unordered_map>
#include <iostream>

enum class Options {
	MAX_BOUNCES,
	SAMPLES,
	FPS_LIMIT,
	TILE_WIDTH,
	TILE_HEIGHT,
	W_WIDTH,
	W_HEIGHT,
	THREADS,
};

class OptionsMap{
	public:
		static OptionsMap *Instance(){
			if(instance == nullptr){
				instance = new OptionsMap();
			}
			return instance;
		}

		inline void destroy(){
			delete instance;
		}

		inline void setOption(Options opt, int v){
			opts[opt] = v;
		}

		inline int getOption(Options opt){
			return opts[opt];
		}

		inline void printOptions(){
			std::cout << "MAX_BOUNCES: \t" << opts[Options::MAX_BOUNCES] << std::endl;
			std::cout << "SAMPLES: \t" << opts[Options::SAMPLES] << std::endl;
			std::cout << "FPS_LIMIT: \t" << opts[Options::FPS_LIMIT] << std::endl;
			std::cout << "W_WIDTH: \t" << opts[Options::W_WIDTH] << std::endl;
			std::cout << "W_HEIGHT: \t" << opts[Options::W_HEIGHT] << std::endl;
			std::cout << "TILE_WIDTH: \t" << opts[Options::TILE_WIDTH] << std::endl;
			std::cout << "TILE_HEIGHT: \t" << opts[Options::TILE_HEIGHT] << std::endl;
			std::cout << "THREADS: \t" << opts[Options::THREADS] << std::endl;
		}


	private:
		std::unordered_map<Options, int> opts;

		OptionsMap(){
			opts[Options::MAX_BOUNCES] = 2;
			opts[Options::SAMPLES] = 1;
			opts[Options::FPS_LIMIT] = 60;
			opts[Options::W_WIDTH] = 640;
			opts[Options::W_HEIGHT] = 384;
			opts[Options::TILE_WIDTH] = 640;
			opts[Options::TILE_HEIGHT] = 384;
			opts[Options::THREADS] = 1;
		};

		~OptionsMap(){};

		static OptionsMap* instance;
};

#endif
