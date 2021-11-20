#include "camera.hpp"
#include "glad/glad.h"
#include "glm/gtc/random.hpp"
#include "hittables/sphere.hpp"
#include "hittables/plane.hpp"
#include "options_manager.hpp"
#include "renderer.hpp"
#include <algorithm>
#include <iostream>

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
	glClearColor(0,0,0,0);
	Camera camera(glm::dvec3{0.0, 0.0, 1.0}, glm::dvec3{0.0, 0.0, 0.0}, 2.0);
	const int maxBounces = OptionsMap::Instance()->getOption(Options::MAX_BOUNCES);
	const int samples = OptionsMap::Instance()->getOption(Options::SAMPLES);

	const double fpsLimit = 1.0 / static_cast<double>(OptionsMap::Instance()->getOption(Options::FPS_LIMIT));
	double lastUpdateTime = 0;  // number of seconds since the last loop
	double lastFrameTime = 0;   // number of seconds since the last frame

	while(!glfwWindowShouldClose(this->window)){
		double now = glfwGetTime();
		double deltaTime = now - lastUpdateTime;
		glfwPollEvents();

		for (int row = W_HEIGHT - 1; row >= 0; --row) {
			for (int col = 0; col < W_WIDTH; ++col) {
				Color pxColor(0,0,0);
				for(int s = 0; s < samples; ++s){
					Ray ray = camera.generateCameraRay(col, row);
					pxColor += trace(ray, maxBounces);
				}
				pxColor = pxColor / static_cast<double>(samples);
				putPixel(row, col, pxColor);
			}
		}

		if ((now - lastFrameTime) >= fpsLimit)
		{
			glBindTexture(GL_TEXTURE_2D, this->texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W_WIDTH, W_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, this->frameBuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindVertexArray(this->VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glfwSwapBuffers(this->window);
			std::cout << "Last frame in: " << (now - lastFrameTime) <<std::endl;
			lastFrameTime = now;
		}
		// set lastUpdateTime every iteration
		lastUpdateTime = now;
	}

	glDeleteShader(this->shader);
	glDeleteBuffers(1, &this->VBO);
	glDeleteVertexArrays(1, &this->VAO);
	return true;
}

Color Renderer::trace(Ray &ray, int bounces){
	HitRecord hr;
	if(!scene || bounces <= 0)
		return Color{0,0,0};
	if(scene->traverse(ray, 0.001, INF, hr)){
		Ray scattered;
		Color attenuation;
		if(hr.material->scatter(ray, hr, attenuation, scattered))
			return attenuation * trace(scattered, bounces - 1);
		return Color{0,0,0};
	}
	double t = 0.5*(ray.getDirection().y + 1.0);
	return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
}

void Renderer::putPixel(int row, int col, Color& color){
	int idx = W_WIDTH * row + col;
	unsigned char r = static_cast<unsigned char>(std::clamp(color.r, 0.0, 0.999) * 255.999);
	unsigned char g = static_cast<unsigned char>(std::clamp(color.g, 0.0, 0.999) * 255.999);
	unsigned char b = static_cast<unsigned char>(std::clamp(color.b, 0.0, 0.999) * 255.999);
	this->frameBuffer[idx] = r << 16 | g << 8 | b << 0;
}

void Renderer::setScene(ScenePtr scene){
	this->scene = scene;
}
