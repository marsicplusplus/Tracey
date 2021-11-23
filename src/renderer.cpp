#include "camera.hpp"
#include "glad/glad.h"
#include "glm/gtc/random.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "input_manager.hpp"
#include "options_manager.hpp"
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

	CHECK_ERROR(this->window = glfwCreateWindow(W_WIDTH, W_HEIGHT, title.c_str(), NULL, NULL), "ERROR::Renderer::initSystems > could not create GLFW3 window\n", false)

	glfwMakeContextCurrent(window);

	CHECK_ERROR(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD", false);

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W_WIDTH, W_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	return true;
}

bool Renderer::start() {
	const int tWidth = OptionsMap::Instance()->getOption(Options::TILE_WIDTH);
	const int tHeight = OptionsMap::Instance()->getOption(Options::TILE_HEIGHT);
	if(W_WIDTH % tWidth != 0 || W_HEIGHT % tHeight != 0){
		throw new std::invalid_argument("Window width and height must be multiples of tiles size!");
	}

	glClearColor(0,0,0,0);

	const int maxBounces = OptionsMap::Instance()->getOption(Options::MAX_BOUNCES);
	const int samples = OptionsMap::Instance()->getOption(Options::SAMPLES);
	const double fpsLimit = 1.0 / static_cast<double>(OptionsMap::Instance()->getOption(Options::FPS_LIMIT));
	double lastUpdateTime = 0;  // number of seconds since the last loop

	int currMaxBounces = 5;
	int currSamples = 2;
	const int horizontalTiles = W_WIDTH / tWidth;
	const int verticalTiles = W_HEIGHT / tHeight;

	while(!glfwWindowShouldClose(this->window)){
		double now = glfwGetTime();
		glfwPollEvents();
		handleInput();

		if(scene->update(lastUpdateTime)){
			isBufferInvalid = true;
			currMaxBounces = 5;
			currSamples = 2;
		}

		if(isBufferInvalid){
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
												for(int s = 0; s < currSamples; ++s){
												double u = static_cast<double>(x + randomDouble(gen, 0.0,1.0)) / static_cast<double>(W_WIDTH - 1);
												double v = static_cast<double>(y + randomDouble(gen, 0.0,1.0)) / static_cast<double>(W_HEIGHT - 1);

												CameraPtr cam = scene->getCamera();
												if(cam){
													Ray ray = cam->generateCameraRay(u, v);
													pxColor += trace(ray, currMaxBounces, scene, gen);
												}
											}
										pxColor = pxColor / static_cast<double>(currSamples);
										putPixel(frameBuffer, W_WIDTH * (y) + (x), pxColor);
									}
								}
							}));
				}
			}

			for(auto &f : futures){
				f.get();
			}
			if(currSamples == samples && currMaxBounces == maxBounces){
				isBufferInvalid = false;
			}
			lastUpdateTime = glfwGetTime() - now;
			std::cout << "Last frame in: " << lastUpdateTime << std::endl;

			currSamples = std::clamp(currSamples + 5, 1, samples);
			currMaxBounces = std::clamp(maxBounces + 5, 2, maxBounces);
		}

		glBindTexture(GL_TEXTURE_2D, this->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W_WIDTH, W_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, this->frameBuffer);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(this->window);
	}

	glDeleteShader(this->shader);
	glDeleteBuffers(1, &this->VBO);
	glDeleteVertexArrays(1, &this->VAO);
	return true;
}

Color Renderer::trace(Ray &ray, int bounces, const ScenePtr scene, std::mt19937 &gen){
	HitRecord hr;
	if(!scene || bounces <= 0)
		return Color{0,0,0};
	if(scene->traverse(ray, 0.001, INF, hr)){
		Ray scattered;
		Color attenuation;
		if(hr.material->scatter(ray, hr, attenuation, scattered, gen))
			return attenuation * trace(scattered, bounces - 1, scene, gen);
		return Color{0,0,0};
	}
	double t = 0.5*(ray.getDirection().y + 1.0);
	return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
}

void Renderer::putPixel(uint32_t fb[], int idx, Color& color){
	unsigned char r = static_cast<unsigned char>(std::clamp(color.r, 0.0, 0.999) * 255.999);
	unsigned char g = static_cast<unsigned char>(std::clamp(color.g, 0.0, 0.999) * 255.999);
	unsigned char b = static_cast<unsigned char>(std::clamp(color.b, 0.0, 0.999) * 255.999);
	fb[idx] = r << 16 | g << 8 | b << 0;
}

void Renderer::setScene(ScenePtr scene){
	this->scene = scene;
}

void Renderer::handleInput(){
	InputManager::Instance()->setKeyValue(GLFW_KEY_D, glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_A, glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_W, glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_S, glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_Q, glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS);
	InputManager::Instance()->setKeyValue(GLFW_KEY_E, glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS);
}
