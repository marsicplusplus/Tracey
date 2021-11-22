#ifndef __MATERIALS_MANAGER_HPP__
#define __MATERIALS_MANAGER_HPP__

#include <unordered_map>
#include <iostream>
#include "materials/material.hpp"

class MaterialsManager{
	public:
		static MaterialsManager *Instance(){
			if(instance == nullptr){
				instance = new MaterialsManager();
			}
			return instance;
		}

		inline void destroy(){
			delete instance;
		}

		inline std::shared_ptr<Material> loadMaterial(int i, std::shared_ptr<Material> mat){
			mats[i] = mat;
			return mat;
		}

		inline std::shared_ptr<Material> getMaterial(int mat){
			return mats[mat];
		}

	private:
		std::unordered_map<int, std::shared_ptr<Material>> mats;

		MaterialsManager(){
		};

		~MaterialsManager(){};

		static MaterialsManager* instance;
};

#endif
