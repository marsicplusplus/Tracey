#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "ray.hpp"
#include "defs.hpp"
#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "thread_pool.hpp"
#include <string>
#include <random>

class Renderer{
	public:
		Renderer(const std::string &_title) : title{_title}, pool{std::thread::hardware_concurrency()}, isBufferInvalid(true){};
		~Renderer();

		bool init();
		bool start();
		void setScene(ScenePtr scene);

	private:
		static void putPixel(uint32_t fb[], int idx, Color &color);
		static Color trace(Ray &ray, int bounces, ScenePtr scene, std::mt19937 &gen);
		void handleInput();

		static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

		GLFWwindow *window;
		std::string title;
		unsigned int VBO, VAO, EBO;
		unsigned int shader;
		unsigned int texture;
		ScenePtr scene;
		ThreadPool pool;
		bool isBufferInvalid;
};

#endif
