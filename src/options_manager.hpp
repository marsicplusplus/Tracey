#ifndef __OPTIONS_MANAGER_HPP__
#define __OPTIONS_MANAGER_HPP__

#include "defs.hpp"
#include <unordered_map>
#include <iostream>

namespace Options{
	enum Options {
		MAX_BOUNCES,
		SAMPLES,
		FPS_LIMIT,
		TILE_WIDTH,
		TILE_HEIGHT,
	};
}

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

		inline void setOption(Options::Options opt, int v){
			opts[opt] = v;
		}

		inline int getOption(Options::Options opt){
			return opts[opt];
		}

		inline void printOptions(){
			std::cout << "MAX_BOUNCES: \t" << opts[Options::MAX_BOUNCES] << std::endl;
			std::cout << "SAMPLES: \t" << opts[Options::SAMPLES] << std::endl;
			std::cout << "FPS_LIMIT: \t" << opts[Options::FPS_LIMIT] << std::endl;
			std::cout << "TILE_WIDTH: \t" << opts[Options::TILE_WIDTH] << std::endl;
			std::cout << "TILE_HEIGHT: \t" << opts[Options::TILE_HEIGHT] << std::endl;
		}


	private:
		std::unordered_map<Options::Options, int> opts;

		OptionsMap(){
			opts[Options::MAX_BOUNCES] = 10;
			opts[Options::SAMPLES] = 10;
			opts[Options::FPS_LIMIT] = 60;
			opts[Options::TILE_WIDTH] = W_WIDTH;
			opts[Options::TILE_HEIGHT] = W_HEIGHT;
		};

		~OptionsMap(){};

		static OptionsMap* instance;
};

#endif
