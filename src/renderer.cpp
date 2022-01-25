#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "camera.hpp"
#include "glad/glad.h"
#include "glm/gtc/random.hpp"
#include "input_manager.hpp"
#include "scene_parser.hpp"
#include "options_manager.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "renderer.hpp"
#include "stb_image_write.h"
#include "stb_image.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>

namespace{
	const char *vertexShaderSource = "#version 450 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec2 aTexturePos;\n"
		"out vec2 TexCoord;\n"
		"void main()\n"
		"{\n"
		"   TexCoord = aTexturePos;"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	const char *fragmentShaderSource = "#version 450 core\n"
		"uniform sampler2D text;\n"
		"in vec2 TexCoord;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
			"FragColor = texture(text, TexCoord);\n"
		"}\0";
};


bool Renderer::init(){
	CHECK_ERROR(glfwInit(), "ERROR::Renderer::initSystems > Cannot initialize glfw\n", false)

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	float scaling = OptionsMap::Instance()->getOption(Options::SCALING);
	if(scaling < 1){
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		CHECK_ERROR(this->window = glfwCreateWindow(mode->width, mode->height, title.c_str(), NULL, NULL), "ERROR::Renderer::initSystems > could not create GLFW3 window\n", false)
	} else {
		CHECK_ERROR(this->window = glfwCreateWindow(OptionsMap::Instance()->getOption(Options::W_WIDTH)*OptionsMap::Instance()->getOption(Options::SCALING), OptionsMap::Instance()->getOption(Options::W_HEIGHT)*OptionsMap::Instance()->getOption(Options::SCALING), title.c_str(), NULL, NULL), "ERROR::Renderer::initSystems > could not create GLFW3 window\n", false)
	}
	glfwMakeContextCurrent(this->window);
	glfwSetMouseButtonCallback(this->window, mouseCallback);
	glfwSetScrollCallback(this->window, scrollCallback);
	glfwSetWindowUserPointer(this->window, this);
	glfwSetKeyCallback(this->window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
		InputManager::Instance()->setKeyValue(key, action != GLFW_RELEASE);
		if(key == GLFW_KEY_T && action == GLFW_PRESS){
			static_cast<Renderer*>(glfwGetWindowUserPointer(w))->toggleGui();
		}
	});

	CHECK_ERROR(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD", false);

	initGui();

	stbi_flip_vertically_on_write(true);

	/* Shader */
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	this->quadShader = glCreateProgram();
	glAttachShader(this->quadShader, vertexShader);
	glAttachShader(this->quadShader, fragmentShader);
	glLinkProgram(this->quadShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	if(OptionsMap::Instance()->getOption(Options::USE_GPU) > 0){
		if(loadComputeShaders()){
			useComputeShader = true;
		}
	}

	/* Quad */
	float vertices[] = {
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // top right
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f, // top left
	};
	unsigned int indices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
	glBindVertexArray(this->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// vertex positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Texture Coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	/* Texture */
	glGenTextures(1, &this->texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, OptionsMap::Instance()->getOption(Options::W_WIDTH), OptionsMap::Instance()->getOption(Options::W_HEIGHT), 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindImageTexture(0, this->texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	
	this->nSamples = OptionsMap::Instance()->getOption(Options::SAMPLES);
	this->nBounces = OptionsMap::Instance()->getOption(Options::MAX_BOUNCES);
	return true;
}

void Renderer::initGui() {

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(this->window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	this->showGui = true;
	this->guiK1 = 1;
	this->guiK2 = 1;
	this->guiBarrel = false;
	this->guiFisheye = false;
	this->guiFisheyeAngle = glm::radians(90.0f);
	this->guiContinuousRender = false;
	this->guiCamPos = glm::fvec3(0, 0, 1);
	this->guiCamDir = glm::fvec3(0, 0, -1);
	this->guiGammaCorrection = false;
	this->guiAberration = false;
	this->aberrationOffset = glm::fvec3(0.0f, 0.0f, 0.0f);
	this->guiVignetting = false;
	this->vignettingSlider = 1.0f;
	this->fBrowser.SetTitle("Choose a scene");
	this->fBrowser.SetWindowSize(500, 300);
	this->fBrowser.SetTypeFilters({".json"});
}

bool Renderer::coreRayTracing(int horizontalTiles, int verticalTiles, int tWidth, int tHeight, int wWidth, int wHeight) {
	std::vector<std::future<void>> futures;
	for(int tileRow = 0; tileRow < verticalTiles; ++tileRow){
		for(int tileCol = 0; tileCol < horizontalTiles; ++tileCol){
			/* Launch thread */
			int samples = this->nSamples;
			int bounces = this->nBounces;
			futures.push_back(Threading::pool.queue([&, tileRow, tileCol, samples, bounces](uint32_t &rng){
				CameraPtr cam = scene->getCamera();
				for (int row = 0; row < tHeight; ++row) {
					for (int col = 0; col < tWidth; ++col) {
						Color pxColor(0,0,0);
						int x = col + tWidth * tileCol;
						int y = row + tHeight * tileRow;
						if (cam) {
							for(int s = 0; s < samples; ++s){
								float u = static_cast<float>(x + ((samples > 1) ? Random::RandomFloat(rng) : 0)) / static_cast<float>(wWidth - 1);
								float v = static_cast<float>(y + ((samples > 1) ? Random::RandomFloat(rng) : 0)) / static_cast<float>(wHeight - 1);
								Ray ray = cam->generateCameraRay(u, v);
								if (ray.getDirection() == glm::fvec3(0, 0, 0)) {
									pxColor += Color(0, 0, 0);
								} else {
									pxColor += Core::traceWhitted(ray, bounces, scene, rng);
								}
							}
						}
						pxColor = pxColor / static_cast<float>(samples);
						int idx = wWidth * y + x;
						putPixel(frameBuffer, idx, pxColor);
					}
				}
			}));
		}
	}

	for (auto& f : futures) 
		f.get();

	isBufferInvalid = false;
	if(saveFrames) saveCurrentFrame(this->nFrames++);
	return true;
}

bool Renderer::start() {
	const int tWidth = OptionsMap::Instance()->getOption(Options::TILE_WIDTH);
	const int tHeight = OptionsMap::Instance()->getOption(Options::TILE_HEIGHT);
	const int wHeight = OptionsMap::Instance()->getOption(Options::W_HEIGHT);
	const int wWidth = OptionsMap::Instance()->getOption(Options::W_WIDTH);
	if(wWidth % tWidth != 0 || wHeight % tHeight != 0){
		throw new std::invalid_argument("Window width and height must be multiples of tiles size!");
	}
	
	glClearColor(0,0,0,0);

	const float fpsLimit = 1.0 / static_cast<float>(OptionsMap::Instance()->getOption(Options::FPS_LIMIT));
	this->lastUpdateTime = glfwGetTime();  // number of seconds since the last loop
	float frameTime = 0.0f;
	float lasttime = 0.0f;

	const int horizontalTiles = wWidth / tWidth;
	const int verticalTiles = wHeight / tHeight;

#ifdef DEBUG
	std::deque<float> averageFrameTime;
#endif
	while(!glfwWindowShouldClose(this->window)){

		if(scene && (this->isBufferInvalid)) {
			if(this->useComputeShader){
				glUseProgram(this->megaKernel);
				bindBuffers();
				glDispatchCompute(wWidth, wHeight, 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				/*  Should we use memoryBarrier between "the waves"? The guidelines on https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glMemoryBarrier.xhtml say:
				 * - Data that are read-only or constant may be accessed without using coherent variables or calling MemoryBarrier(). 
				 * 		Updates to the read-only data via API calls such as glBufferSubData will invalidate shader caches implicitly as required.
				 * - Data that are shared between shader invocations at a fine granularity (e.g., written by one invocation, consumed by another invocation) should use coherent variables to read and write the shared data.
				 * - Data written by one shader invocation and consumed by other shader invocations launched as a result of its execution ("dependent invocations") should 
				 *   	use coherent variables in the producing shader invocation and call memoryBarrier() after the last write. The consuming shader invocation should also use coherent variables.
				 * - Data written to image variables in one rendering pass and read by the shader in a later pass need not use coherent variables or memoryBarrier(). 
				 *   	Calling glMemoryBarrier with the SHADER_IMAGE_ACCESS_BARRIER_BIT set in barriers between passes is necessary.
				 * - Data written by the shader in one rendering pass and read by another mechanism (e.g., vertex or index buffer pulling) 
				 *   	in a later pass need not use coherent variables or memoryBarrier(). Calling glMemoryBarrier with the appropriate bits set in barriers between passes is necessary. 
				*/
				//glUseProgram(this->generateKernel);
				//glDispatchCompute(wWidth, wHeight, 1);
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				//glUseProgram(this->extendKernel);
				//glDispatchCompute(wWidth, wHeight, 1);
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				//glUseProgram(this->shadeKernel);
				//glDispatchCompute(wWidth, wHeight, 1);
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				//glUseProgram(this->connectKernel);
				//glDispatchCompute(wWidth, wHeight, 1);
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			} else {
				this->coreRayTracing(horizontalTiles, verticalTiles, tWidth, tHeight, wWidth, wHeight);
			}
			std::cout << "Last frameTime: " << lasttime << "s" << std::endl;
#ifdef DEBUG
			averageFrameTime.push_back(lasttime);
			int count = averageFrameTime.size();
			if(count > 500) averageFrameTime.pop_front();
			std::cout << "Average time of the last " << count << " frames: " << 
				std::reduce(averageFrameTime.begin(), averageFrameTime.end()) / count << "s" << std::endl;
#endif
		}
		float now = glfwGetTime();
		frameTime = now - lastUpdateTime;
		lasttime = frameTime;
		lastUpdateTime = now;

		while(frameTime > 0.0f){
			float dt = min(frameTime, fpsLimit);
			frameTime -= dt;
			glfwPollEvents();
			double xpos, ypos;
			glfwGetCursorPos(this->window, &xpos, &ypos);
			InputManager::Instance()->setMouseState(xpos, ypos);
			if(scene) this->isBufferInvalid = this->scene->update(dt);
		}

		glUseProgram(this->quadShader);
		uint32_t* buffer = applyPostProcessing();
		glBindTexture(GL_TEXTURE_2D, this->texture);
		if(!useComputeShader){
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wWidth, wHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		if (this->showGui)
			renderGUI();
		glfwSwapBuffers(this->window);
	}

	return true;
}

void Renderer::putPixel(uint32_t fb[], int idx, uint8_t r, uint8_t g, uint8_t b){
	fb[idx] = r << 16 | g << 8 | b << 0;
}

void Renderer::putPixel(uint32_t fb[], int idx, Color& color){
	unsigned char r = static_cast<unsigned char>(std::clamp(color.r, 0.0f, 0.999f) * 255.999f);
	unsigned char g = static_cast<unsigned char>(std::clamp(color.g, 0.0f, 0.999f) * 255.999f);
	unsigned char b = static_cast<unsigned char>(std::clamp(color.b, 0.0f, 0.999f) * 255.999f);
	fb[idx] = r << 16 | g << 8 | b << 0;
}

void Renderer::setScene(ScenePtr scene){
	this->scene = scene;
	this->isBufferInvalid = true;
	if(useComputeShader){
		scene->getMeshBuffer(tris, nodes, meshes);
		scene->getInstanceBuffer(instances);
		scene->getTextureBuffer(textures, imgs);
		scene->getMaterialBuffer(materials);
		scene->getLightBuffer(lights);
		genBuffers();
	}
}

void Renderer::mouseCallback(GLFWwindow* window, int button, int action, int mods) {
		InputManager::Instance()->setKeyValue(button, action == GLFW_PRESS);
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	InputManager::Instance()->scrollState(yoffset);
}

void Renderer::renderGUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(.0f, .0f));
	ImGui::SetNextWindowSize(ImVec2((OptionsMap::Instance()->getOption(Options::W_WIDTH)/3) * OptionsMap::Instance()->getOption(Options::SCALING) , OptionsMap::Instance()->getOption(Options::W_HEIGHT) * OptionsMap::Instance()->getOption(Options::SCALING)));
	{
		ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

		if (ImGui::CollapsingHeader("Scene")) {
			if (ImGui::Button("Load Scene")) {
				this->fBrowser.Open();
			}
			if(scene){
				ImGui::TextWrapped("Num of triangles: %d", scene->getNTris());
				//ImGui::TextWrapped("BVH Building time: %d", scene->getNTris());
			}
		}
		if(scene){
			if (ImGui::CollapsingHeader("Camera Settings")) {

				ImGui::Spacing();
				ImGui::Spacing();

				guiSensitivity = this->scene->getCamera()->getSensitivity();
				ImGui::TextWrapped("Camera Sensitivity");
				if (ImGui::SliderFloat("##CameraSensitivity", &guiSensitivity, 1, 20)) {
					this->scene->getCamera()->setSensitivity(guiSensitivity);
				}

				ImGui::Spacing();
				ImGui::Spacing();
				guiFOV = glm::degrees(this->scene->getCamera()->getFOV());
				ImGui::TextWrapped("FOV");
				if (ImGui::SliderInt("##FOV", &guiFOV, 20, 120)) {
					this->scene->getCamera()->setFOV(guiFOV);
				}

				ImGui::Spacing();
				ImGui::Spacing();
				guiCamPos = this->scene->getCamera()->getPosition();
				ImGui::Text("Position");
				if (ImGui::InputFloat3("##Position", &guiCamPos[0], "%.2f")) {
					this->scene->getCamera()->setPosition(guiCamPos);
				}

				ImGui::Spacing();
				ImGui::Spacing();
				guiCamDir = this->scene->getCamera()->getDirection();
				ImGui::Text("Direction");
				if (ImGui::InputFloat3("##Direction", &guiCamDir[0], "%.2f")) {
					this->scene->getCamera()->setDirection(guiCamDir, false);
				}

				ImGui::Spacing();
				ImGui::Spacing();
				if (ImGui::Checkbox("Barrel Distorion", &guiBarrel)) {
					this->scene->getCamera()->setCameraType(CameraType::barrel);
					if (guiBarrel && guiFisheye) {
						guiFisheye = false;
					}
					else if (!guiBarrel && !guiFisheye) {
						this->scene->getCamera()->setCameraType(CameraType::normal);
					}
				}

				if (guiBarrel) {
					ImGui::TextWrapped("r'=r*(1 + k1*r^2 + k2*r^4)");
					if (ImGui::SliderFloat("K1", &guiK1, -10.0f, 10.0f, "%.2f")) {
						this->scene->getCamera()->setDistortionCoefficients(guiK1, guiK2);
					}
					if (ImGui::SliderFloat("K2", &guiK2, -10.0f, 10.0f, "%.2f")) {
						this->scene->getCamera()->setDistortionCoefficients(guiK1, guiK2);
					}
				}

				ImGui::Spacing();
				ImGui::Spacing();
				if (ImGui::Checkbox("Fisheye Lens", &guiFisheye)) {
					this->scene->getCamera()->setCameraType(CameraType::fisheye);
					if (guiBarrel && guiFisheye) {
						guiBarrel = false;
					}
					else if (!guiBarrel && !guiFisheye) {
						this->scene->getCamera()->setCameraType(CameraType::normal);
					}
				}

				if (guiFisheye) {
					if (ImGui::SliderAngle("Fisheye Angle", &guiFisheyeAngle, 0, 180, "%.2f deg")) {
						this->scene->getCamera()->setFisheyeAngle(guiFisheyeAngle);
					}
				}

				ImGui::Spacing();
				ImGui::Spacing();
			}


			if (ImGui::CollapsingHeader("Rendering Settings")) {
				ImGui::Spacing();
				ImGui::Spacing();

				if (ImGui::Checkbox("Render Continuously", &guiContinuousRender)) {
					if (guiContinuousRender) {
						this->scene->getCamera()->update(0, true);
					}
				}

				ImGui::TextWrapped("Samples");
				ImGui::SliderInt("##SAMPLES", &nSamples, 1, 100);
				ImGui::TextWrapped("Bounces");
				ImGui::SliderInt("##BOUNCES", &nBounces, 2, 100);
			}
			if(ImGui::CollapsingHeader("PostProcessing Effects")){
				ImGui::Checkbox("Gamma Correction", &guiGammaCorrection);
				ImGui::Checkbox("Vignetting", &guiVignetting);
				if(guiVignetting) ImGui::SliderFloat("##VignetteFactor", &vignettingSlider, 0.0, 2.0f);
				ImGui::Checkbox("Chromatic Aberration", &guiAberration);
				if(guiAberration) ImGui::SliderFloat3("##AberrationOffset", &aberrationOffset[0], -10.00f, 10.0f, "%.3f");
			}
			ImGui::Spacing();
			ImGui::Spacing();
			if (ImGui::Button("Render New Frame")) {
				this->scene->getCamera()->update(0, true);
				this->isBufferInvalid = true;
			}
			ImGui::Spacing();
			if (ImGui::Button("Save current Frame")) {
				saveCurrentFrame(this->nFrames++);
			}

			if (guiContinuousRender) {
				this->isBufferInvalid = true;
			}
		}
		ImGui::End();
		this->fBrowser.Display();
		if(this->fBrowser.HasSelected())
		{
			this->setScene(std::make_shared<Scene>(this->fBrowser.GetSelected().string()));
			this->fBrowser.ClearSelected();
			this->lastUpdateTime = glfwGetTime();
		}
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Renderer::~Renderer() {
	delete[] this->frameBuffer;
	delete[] this->secondaryBuffer;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(this->window);
	glDeleteShader(this->quadShader);
	glDeleteShader(this->megaKernel);
	//glDeleteShader(this->generateKernel);
	//glDeleteShader(this->extendKernel);
	//glDeleteShader(this->shadeKernel);
	//glDeleteShader(this->connectKernel);
	glDeleteBuffers(1, &this->VBO);
	glDeleteVertexArrays(1, &this->VAO);
}

uint32_t* Renderer::applyPostProcessing(){
	int wWidth = OptionsMap::Instance()->getOption(Options::W_WIDTH);
	int wHeight = OptionsMap::Instance()->getOption(Options::W_HEIGHT);
	if(guiGammaCorrection || guiVignetting || guiAberration){
		glm::fvec2 center(wWidth/2, wHeight/2);
		for(size_t y = 0; y < wHeight; ++y){
			for(size_t x = 0; x < wWidth; ++x){
				int idx = y * wWidth + x;
				float dist = sqrt((x-center.x)*(x-center.x) + (y-center.y)*(y-center.y)); 
				float scaledDist = dist / sqrt(wWidth/2*wWidth/2 + wHeight/2*wHeight/2);
				uint32_t pixel = frameBuffer[idx];
				float r = static_cast<uint8_t>(pixel >> 16) / 255.0f;
				float g = static_cast<uint8_t>(pixel >> 8) / 255.0f;
				float b = static_cast<uint8_t>(pixel >> 0) / 255.0f;
				Color c(r,g,b);
				if(guiAberration){
					int nX = static_cast<int>(x + scaledDist * aberrationOffset.r) % wWidth;
					c.r = static_cast<uint8_t>(frameBuffer[y * wWidth + nX] >> 16)/255.0f;
					nX = static_cast<int>(x + scaledDist * aberrationOffset.g) % wWidth;
					c.g = static_cast<uint8_t>(frameBuffer[y * wWidth + nX] >> 8)/255.0f;
					nX = static_cast<int>(x + scaledDist * aberrationOffset.b) % wWidth;
					c.b = static_cast<uint8_t>(frameBuffer[y * wWidth + nX] >> 0)/255.0f;
				}
				if(guiGammaCorrection){
					c.r = sqrt(c.r);
					c.g = sqrt(c.g);
					c.b = sqrt(c.b);
				}
				if(guiVignetting){
					c = (Color(0.0,0.0,0.0) - c) * (scaledDist*this->vignettingSlider) + c;
				}
				putPixel(secondaryBuffer, idx, c);
			}
		}
		return this->secondaryBuffer;
	}
	return this->frameBuffer;
}
void Renderer::saveCurrentFrame(int frame){
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,80,"%d%m%Y",timeinfo);
	sprintf(buffer, "%s_%d.png", buffer, frame);

	int wWidth = OptionsMap::Instance()->getOption(Options::W_WIDTH);
	int wHeight = OptionsMap::Instance()->getOption(Options::W_HEIGHT);
	auto *bitmap = new uint8_t[3*wWidth * wHeight];
	int i = 0;
	int k = 0;
	uint32_t *fb = applyPostProcessing();
	while(i < wWidth * wHeight){
		bitmap[k++] = static_cast<uint8_t>(fb[i] >> 16);
		bitmap[k++] = static_cast<uint8_t>(fb[i] >> 8);
		bitmap[k++] = static_cast<uint8_t>(fb[i] >> 0);
		i++;
	}
	stbi_write_png(buffer, wWidth, wHeight, 3, bitmap, 3 * wWidth);
	delete[] bitmap;
}
const std::string Renderer::readShader(std::string path) {
	if(!std::filesystem::exists(path)){
		std::cerr << "Cannot open shader file: " << path << std::endl;
		return nullptr;
	}
	std::string code;
	std::ifstream shaderFile;
	std::stringstream shaderStream;
	shaderFile.open(path);
	if(!shaderStream){
		std::cerr << "Cannot read shader file: " << path << std::endl;
		return nullptr;
	}
	shaderStream << shaderFile.rdbuf();
	shaderFile.close();
	code = shaderStream.str();
	return code;
}

bool Renderer::loadComputeShaders(){
	std::string code = readShader("kernels/mega_kernel.glsl");
	if(code.empty()) return false;
	auto codeChar = code.c_str();
	unsigned int megaKernelShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(megaKernelShader, 1, &codeChar, NULL);
	glCompileShader(megaKernelShader);

	GLint isCompiled = 0;
	glGetShaderiv(megaKernelShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(megaKernelShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(megaKernelShader, maxLength, &maxLength, &errorLog[0]);

		// Exit with failure.
		for (char i : errorLog)
			std::cout << i;
		std::cout << std::endl;
		glDeleteShader(megaKernelShader); // Don't leak the shader.
		return false;
	}
	megaKernel = glCreateProgram();
	glAttachShader(megaKernel, megaKernelShader);
	glLinkProgram(megaKernel);
	glDeleteShader(megaKernelShader);

	return true;
}

void Renderer::bindBuffers() {

	glBindImageTexture(0, this->texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, textureSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, triSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, nodeSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, materialSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, lightSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, imageSSBO);

	auto cam = this->scene->getCamera();
	glUniform3f(glGetUniformLocation(megaKernel, "camPosition"), cam->position[0], cam->position[1], cam->position[2]);
	glUniform3f(glGetUniformLocation(megaKernel, "llCorner"), cam->llCorner[0], cam->llCorner[1], cam->llCorner[2]);
	glUniform3f(glGetUniformLocation(megaKernel, "horizontal"), cam->horizontal[0], cam->horizontal[1], cam->horizontal[2]);
	glUniform3f(glGetUniformLocation(megaKernel, "vertical"), cam->vertical[0], cam->vertical[1], cam->vertical[2]);
	glUniform1i(glGetUniformLocation(megaKernel, "bounces"), nBounces);
}

void Renderer::genBuffers() {
	glGenBuffers(1, &meshSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, meshSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CompactMesh) * meshes.size(), &meshes[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &instanceSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, instanceSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Instance) * instances.size(), &instances[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instanceSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &textureSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, textureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CompactTexture) * textures.size(), &textures[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, textureSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &triSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CompactTriangle) * tris.size(), &tris[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, triSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &nodeSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BVHNode) * nodes.size(), &nodes[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, nodeSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CompactMaterial) * materials.size(), &materials[0], GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &lightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(CompactLight) * lights.size(), &lights[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, lightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	glGenBuffers(1, &imageSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, imageSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * imgs.size(), &imgs[0], GL_DYNAMIC_COPY); 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, imageSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
}
