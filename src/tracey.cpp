#include "renderer.hpp"

int main(int argc, char *args[]){
	Renderer renderer("TraceyGL");
	
	renderer.init();
	renderer.start();
}
