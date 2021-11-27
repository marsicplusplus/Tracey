#include "camera.hpp"
#include "glad/glad.h"
#include "glm/gtc/random.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "input_manager.hpp"
#include "options_manager.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "renderer.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

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

	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	CHECK_ERROR(this->window = glfwCreateWindow(OptionsMap::Instance()->getOption(Options::W_WIDTH), OptionsMap::Instance()->getOption(Options::W_HEIGHT), title.c_str(), NULL, NULL), "ERROR::Renderer::initSystems > could not create GLFW3 window\n", false)

	glfwMakeContextCurrent(this->window);
	glfwSetMouseButtonCallback(this->window, mouseCallback);
	glfwSetScrollCallback(this->window, scrollCallback);

	CHECK_ERROR(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD", false);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(this->window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	/* Shader */
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	this->shader = glCreateProgram();
	glAttachShader(this->shader, vertexShader);
	glAttachShader(this->shader, fragmentShader);
	glLinkProgram(this->shader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glUseProgram(this->shader);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, OptionsMap::Instance()->getOption(Options::W_WIDTH), OptionsMap::Instance()->getOption(Options::W_HEIGHT), 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
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

	const int maxBounces = OptionsMap::Instance()->getOption(Options::MAX_BOUNCES);
	const int samples = OptionsMap::Instance()->getOption(Options::SAMPLES);
	const double fpsLimit = 1.0 / static_cast<double>(OptionsMap::Instance()->getOption(Options::FPS_LIMIT));
	double lastUpdateTime = 0;  // number of seconds since the last loop

	const int horizontalTiles = wWidth / tWidth;
	const int verticalTiles = wHeight / tHeight;


	while(!glfwWindowShouldClose(this->window)){
		double now = glfwGetTime();
		glfwPollEvents();
		handleInput();

		if (InputManager::Instance()->isKeyDown(GLFW_KEY_T)) {
			this->showGui = true;
		}
		if (InputManager::Instance()->isKeyDown(GLFW_KEY_N)) {
			this->showGui = false;
		}

		if(scene->update(lastUpdateTime)){
			this->isBufferInvalid = true;
		}


		if(this->isBufferInvalid) {
			std::vector<std::future<void>> futures;
			for(int tileRow = 0; tileRow < verticalTiles; ++tileRow){
				for(int tileCol = 0; tileCol < horizontalTiles; ++tileCol){
					/* Launch thread */
					futures.push_back(pool.queue([&, tileRow, tileCol](std::mt19937& gen){
						for (int row = 0; row < tHeight; ++row) {
							for (int col = 0; col < tWidth; ++col) {
								Color pxColor(0,0,0);
								int x = col + tWidth * tileCol;
								int y = row + tHeight * tileRow;
								for(int s = 0; s < samples; ++s){
									double u = static_cast<double>(x + randomDouble(gen, 0.0,1.0)) / static_cast<double>(wWidth - 1);
									double v = static_cast<double>(y + randomDouble(gen, 0.0,1.0)) / static_cast<double>(wHeight - 1);
									CameraPtr cam = scene->getCamera();
									if(cam){
										Ray ray = cam->generateCameraRay(u, v);
										pxColor += trace(ray, maxBounces, scene);
									}
								}
								pxColor = pxColor / static_cast<double>(samples);
								putPixel(frameBuffer, wWidth * (y) + (x), pxColor);
							}
						}
					}));
				}
			}

			for (auto& f : futures) 
				f.get();

			lastUpdateTime = glfwGetTime() - now;
			std::cout << std::endl << "Last frame info:" <<std::endl;
			std::cout << lastUpdateTime << "s" << std::endl;
			std::cout << samples << " samples per pixel" << std::endl;
			std::cout << maxBounces << " maximum number of ray bounces" << std::endl;
			isBufferInvalid = false;
		}


		glBindTexture(GL_TEXTURE_2D, this->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wWidth, wHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, frameBuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		if (this->showGui)
			renderGUI();
		glfwSwapBuffers(this->window);
	}

	return true;
}

Color Renderer::trace(Ray &ray, int bounces, const ScenePtr scene){
	HitRecord hr;
	if(!scene || bounces <= 0)
		return Color{0,0,0};
	if(scene->traverse(ray, 0.001, INF, hr)){
		Ray scattered;
		Color attenuation;
		if(hr.material->getReflective() == 0.0)
			return hr.material->getAlbedo(hr) * scene->traceLights(hr);
		if(hr.material->getReflective() == 1.0){
			Ray reflected(hr.p, ray.getDirection() - 2*glm::dot(ray.getDirection(), hr.normal)*hr.normal);
			return hr.material->getAlbedo(hr) * trace(reflected, bounces - 1, scene);
		}
		return Color{0,0,0};
	}
	return Color(0.3,0.8,0.2);
}

void Renderer::putPixel(uint32_t fb[], int idx, Color& color){
	unsigned char r = static_cast<unsigned char>(std::clamp(sqrt(color.r), 0.0, 0.999) * 255.999);
	unsigned char g = static_cast<unsigned char>(std::clamp(sqrt(color.g), 0.0, 0.999) * 255.999);
	unsigned char b = static_cast<unsigned char>(std::clamp(sqrt(color.b), 0.0, 0.999) * 255.999);
	fb[idx] = r << 16 | g << 8 | b << 0;
}

void Renderer::setScene(ScenePtr scene){
	this->scene = scene;
}

void Renderer::handleInput(){
	double xpos, ypos;
	glfwGetCursorPos(this->window, &xpos, &ypos);
	InputManager::Instance()->setMouseState(xpos, ypos);
	InputManager::Instance()->setKeyValue(GLFW_KEY_D, glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_A, glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_W, glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_S, glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_Q, glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_E, glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_T, glfwGetKey(this->window, GLFW_KEY_T) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_N, glfwGetKey(this->window, GLFW_KEY_N) == GLFW_PRESS);
}

void Renderer::mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		InputManager::Instance()->setKeyValue(GLFW_MOUSE_BUTTON_RIGHT, (action == GLFW_PRESS) ? true : false);
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		InputManager::Instance()->setKeyValue(GLFW_MOUSE_BUTTON_LEFT, (action == GLFW_PRESS) ? true : false);
	}
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	InputManager::Instance()->scrollState(yoffset);
}

void Renderer::renderGUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(.0f, .0f));
	ImGui::SetNextWindowSize(ImVec2(OptionsMap::Instance()->getOption(Options::W_WIDTH) / 3, OptionsMap::Instance()->getOption(Options::W_HEIGHT)));
	{
		ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
		// render your GUI
		guiFOV = glm::degrees(this->scene->getCamera()->getFOV());
		if (ImGui::SliderInt("FOV", &guiFOV, 20, 120)) {
			this->scene->getCamera()->setFOV(guiFOV);
		}

		if (ImGui::Button("Render")) {
			this->scene->getCamera()->update(0, true);
			this->isBufferInvalid = true;
		}

		ImGui::Text("Camera");
		guiCamPos = this->scene->getCamera()->getPosition();
		if (ImGui::InputFloat3("Position", &guiCamPos[0], "%.2f")) {
			this->scene->getCamera()->setPosition(guiCamPos);
		}

		guiCamDir = this->scene->getCamera()->getDirection();
		if (ImGui::InputFloat3("Direction", &guiCamDir[0], "%.2f")) {
			this->scene->getCamera()->setDirection(guiCamDir, false);
		}

		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Renderer::~Renderer() {
	delete(this->frameBuffer);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(this->window);
	pool.cancel_pending();
	glDeleteShader(this->shader);
	glDeleteBuffers(1, &this->VBO);
	glDeleteVertexArrays(1, &this->VAO);
}


