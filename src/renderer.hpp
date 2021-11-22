#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "ray.hpp"
#include "defs.hpp"
#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "thread_pool.h"
#include <string>

struct JobReturn {
	Color col;
	size_t idx;
};

class Renderer{
	public:
		Renderer(const std::string &_title) : title{_title}, pool{1} {};
		inline ~Renderer() {
			glfwDestroyWindow(window);
		}

		bool init();
		bool start();
		void setScene(ScenePtr scene);

	private:
		static void putPixel(uint32_t fb[], int idx, Color &color);
		static Color trace(Ray &ray, int bounces, ScenePtr scene);

		GLFWwindow *window;
		std::string title;
		uint32_t frameBuffer[W_WIDTH * W_HEIGHT];
		unsigned int VBO, VAO, EBO;
		unsigned int shader;
		unsigned int texture;
		ScenePtr scene;
		ThreadPool pool;
};

#endif
