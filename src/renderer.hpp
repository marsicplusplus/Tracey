#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "ray.hpp"
#include "defs.hpp"
#include "scene.hpp"
#include "GLFW/glfw3.h"
#include <string>

class Renderer{
	public:
		Renderer(const std::string &_title) : title{_title} {};
		inline ~Renderer() {
			glfwDestroyWindow(window);
		}

		bool init();
		bool start();
		void setScene(ScenePtr scene);

	private:
		void putPixel(int row, int col, Color &color);
		Color trace(Ray &ray, int bounces);

		GLFWwindow *window;
		std::string title;
		uint32_t frameBuffer[W_WIDTH * W_HEIGHT];
		unsigned int VBO, VAO, EBO;
		unsigned int shader;
		unsigned int texture;
		ScenePtr scene;
};

#endif
