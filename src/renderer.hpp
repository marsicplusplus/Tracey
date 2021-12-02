#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "ray.hpp"
#include "defs.hpp"
#include "scene.hpp"
#include "GLFW/glfw3.h"
#include "options_manager.hpp"
#include "thread_pool.hpp"
#include <string>
#include "imgui.h"
#include "imfilebrowser.h"

class Renderer{
public:
	inline Renderer(const std::string& _title, size_t pool = 1) : title{ _title }, pool{ pool }, isBufferInvalid(false){
			this->frameBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
			this->secondaryBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
		};
		~Renderer();

		bool init();
		void initGui();
		bool start();
		void setScene(ScenePtr scene);

	private:
		static void putPixel(uint32_t fb[], int idx, Color &color);
		static void putPixel(uint32_t fb[], int idx, uint8_t r, uint8_t g, uint8_t b);
		static Color trace(Ray &ray, int bounces, ScenePtr scene);
		void handleInput();

		static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		void renderGUI();
		uint32_t* applyPostProcessing();

		GLFWwindow *window;
		std::string title;
		uint32_t *frameBuffer;
		uint32_t *secondaryBuffer;
		unsigned int VBO, VAO, EBO;
		unsigned int shader;
		unsigned int texture;
		ScenePtr scene;
		ThreadPool pool;
		bool isBufferInvalid;

		bool showGui;
		int guiFOV;
		float guiSensitivity;
		glm::vec3 guiCamPos;
		glm::vec3 guiCamDir;
		float guiK1;
		float guiK2;
		bool guiBarrel;
		bool guiFisheye;
		bool guiContinuousRender;
		bool guiGammaCorrection;
		bool guiVignetting;
		float vignettingSlider;
		bool guiAberration;
		glm::vec3 aberrationOffset;
		float guiFisheyeAngle;
		int nSamples;
		int nBounces;
		ImGui::FileBrowser fBrowser;
};

#endif
