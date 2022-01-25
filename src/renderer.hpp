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
	inline Renderer(std::string  _title, bool saveFrames = false) : title{std::move( _title )}, isBufferInvalid(false), saveFrames(saveFrames), useComputeShader(false) {
			this->frameBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
			this->secondaryBuffer = new uint32_t[OptionsMap::Instance()->getOption(Options::W_WIDTH) * OptionsMap::Instance()->getOption(Options::W_HEIGHT)];
			nFrames = 0;
		};
		~Renderer();

		bool init();
		void initGui();
		bool start();
		inline void toggleGui() {showGui = !showGui;}
		void setScene(ScenePtr scene);

	private:
		bool coreRayTracing(int horizontalTiles, int verticalTiles, int tWidth, int tHeight, int wWidth, int wHeight);

		static void putPixel(uint32_t fb[], int idx, Color &color);
		static void putPixel(uint32_t fb[], int idx, uint8_t r, uint8_t g, uint8_t b);
		void handleInput();
		bool loadComputeShaders(); /* If false is returned, do not use GPU rendering */
		void bindBuffers();
		void genBuffers();
		const std::string readShader(std::string path);

		static void mouseCallback(GLFWwindow* window, int button, int action, int mods);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		void renderGUI();
		uint32_t* applyPostProcessing();
		void saveCurrentFrame(int frame);

		GLFWwindow *window;
		std::string title;
		unsigned int textFrameBuffer;
		uint32_t *frameBuffer;
		uint32_t *secondaryBuffer;
		unsigned int VBO, VAO, EBO;
		unsigned int quadShader;
		unsigned int megaKernel;
		//unsigned int generateKernel;
		//unsigned int extendKernel;
		//unsigned int shadeKernel;
		//unsigned int connectKernel;
		unsigned int texture;
		ScenePtr scene;
		bool isBufferInvalid;
		bool useComputeShader;

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
		bool guiVignetting;
		float vignettingSlider;
		bool guiAberration;
		bool saveFrames;
		glm::vec3 aberrationOffset;
		float guiFisheyeAngle;
		int nSamples;
		int nBounces;
		int nFrames;
		ImGui::FileBrowser fBrowser;

		unsigned int fbSSBO;
		std::vector<CompactMesh> meshes;
		unsigned int meshSSBO;
		std::vector<CompactTriangle> tris;
		unsigned int triSSBO;
		std::vector<BVHNode> nodes;
		unsigned int nodeSSBO;
		std::vector<Instance> instances;
		unsigned int instanceSSBO;
		std::vector<CompactTexture> textures;
		unsigned int textureSSBO;
		std::vector<unsigned int> imgs;
		unsigned int imageSSBO;
		std::vector<CompactMaterial> materials;
		unsigned int materialSSBO;
		std::vector<CompactLight> lights;
		unsigned int lightSSBO;
};

#endif
