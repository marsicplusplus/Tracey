#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "GLFW/glfw3.h"
#include "defs.hpp"
#include "imgui.h"
#include "imfilebrowser.h"
#include "options_manager.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include "thread_pool.hpp"
#include "core.hpp"
#include <string>
#include <utility>

class Renderer{
public:
	inline Renderer(std::string  _title) : title{std::move( _title )}, isBufferInvalid(false), shouldAverage(true) {
			this->frameBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
			this->secondaryBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
			this->accumulator = new Color[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
		};
		~Renderer();

		bool init();
		void initGui();
		bool start();
		inline void toggleGui() {showGui = !showGui;}
		void setScene(ScenePtr scene);

	private:
		bool corePathTracing(int horizontalTiles, int verticalTiles, int tWidth, int tHeight, int wWidth, int wHeight);
		bool coreRayTracing(int horizontalTiles, int verticalTiles, int tWidth, int tHeight, int wWidth, int wHeight);
		bool corePacketRayTracing(int horizontalTiles, int verticalTiles, int tWidth, int tHeight, int wWidth, int wHeight);

		static void putPixel(uint32_t fb[], int idx, Color &color);
		static void putPixel(uint32_t fb[], int idx, uint8_t r, uint8_t g, uint8_t b);
		void packetTrace(std::vector<Ray> &corners, std::vector<RayInfo>& rays, int bounces, const ScenePtr scene);
		void handleInput();

		static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		void renderGUI();
		uint32_t* applyPostProcessing();

		GLFWwindow *window;
		std::string title;
		uint32_t *frameBuffer;
		uint32_t *secondaryBuffer;
		Color *accumulator;
		unsigned int VBO, VAO, EBO;
		unsigned int shader;
		unsigned int texture;
		ScenePtr scene;
		bool isBufferInvalid;

		bool showGui;
		int guiFOV;
		float guiSensitivity;
		glm::vec3 guiCamPos;
		glm::vec3 guiCamDir;
		float lastUpdateTime;
		float guiK1;
		float guiK2;
		bool guiBarrel;
		bool guiFisheye;
		bool guiContinuousRender;
		bool guiGammaCorrection;
		bool guiPacketTraversal;
		bool guiVignetting;
		float vignettingSlider;
		bool guiAberration;
		bool shouldAverage;
		int frames = 0;
		glm::vec3 aberrationOffset;
		float guiFisheyeAngle;
		int nSamples;
		int nBounces;
		ImGui::FileBrowser fBrowser;
};

#endif
