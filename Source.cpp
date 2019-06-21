#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#define W_WIDTH 1280
#define W_HEIGHT 720

#define INPUT_PRESSED 0b01
#define INPUT_HELD 0b10

#define MAPWIDTH 24
#define MAPHEIGHT 24

#define NUM_TEXTURES 8
#define TEX_WIDTH 64
#define TEX_HEIGHT 64

#define MOVESPEED 0.04
#define ROTSPEED 0.15

GLFWwindow* window;
unsigned char inputs[350];
float frameTime;

glm::fvec2 pos, dir, plane;

struct Mouse
{
	glm::dvec2 last;
	glm::dvec2 delta;
	bool first = true;
} mouse_move;

unsigned int shaderID;

void CompileShaders(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
void checkCompileErrors(GLuint shader, std::string type);

bool GetInputPressed(int key) { return inputs[key] & INPUT_PRESSED; }
bool GetInputHeld(int key) { return inputs[key] & INPUT_HELD; }

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int worldMap[MAPWIDTH * MAPHEIGHT] =
{
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7,
  4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7,
  4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,
  4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,
  4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7,
  4,0,4,0,0,0,0,5,5,5,5,5,5,5,5,5,7,7,0,7,7,7,7,7,
  4,0,5,0,0,0,0,5,0,5,0,5,0,5,0,5,7,0,0,0,7,7,7,1,
  4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8,
  4,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,7,1,
  4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8,
  4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1,
  4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1,
  6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6,
  8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,
  6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6,
  4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2,
  4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2,
  4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2,
  4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2,
  4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3
};

int main(int argc, char* argv[])
{

	// ========== GLFW BOILERPLATE ==========

	// Initialisation
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Fix compilation on OS X
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create window
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, "RayCasterOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
	}

	// Set window
	glfwMakeContextCurrent(window);

	// Callbacks
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);

	// Capture mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		std::cout << "Failed to initialize GLAD" << std::endl;

	// ========== RAY CASTING SETUP ==========

	pos = { 22, 13.5 };
	dir = { -1, 0 };
	plane = { 0, 0.66 };

	double time = 0; //time of current frame
	double oldTime = 0; //time of previous frame

	// ========== SHADER COMPILATION ==========

	// Compile
	CompileShaders("./shader.vert", "./shader.frag");

	// Uniforms
	int uniform_dir = glGetUniformLocation(shaderID, "dir");
	int uniform_plane = glGetUniformLocation(shaderID, "plane");
	int uniform_pos = glGetUniformLocation(shaderID, "pos");
	int uniform_worldmap = glGetUniformLocation(shaderID, "worldmap");

	// Set static uniforms
	glUseProgram(shaderID);

	glUniform1iv(uniform_worldmap, MAPWIDTH * MAPHEIGHT, worldMap);



	// ========== VERTEX SETUP ==========

	// Single quad used to draw all pixels
	float vertices[] = {
		 1.0, 1.0, 0.0,  // Top right
		1.0, -1.0, 0.0,  // Bottom right
		-1.0, -1.0, 0.0,  // Bottom left
		-1.0, 1.0, 0.0   // Top left 
	};
	unsigned int indices[] = {
		0, 1, 3,  // First Triangle
		1, 2, 3   // Second Triangle
	};

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	int width, height, nrChannels;
	unsigned char* data = stbi_load("./pics/eagle.png", &width, &height, &nrChannels, 0);
	stbi_image_free(data);


	// ========== GAME LOOP ==========

	bool quit = false;
	while (!quit)
	{
		// Clear previous buffer
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		// Process inputs
		if (GetInputPressed(GLFW_KEY_ESCAPE)) quit = true;

		if (GetInputPressed(GLFW_KEY_W))
		{
			if (worldMap[int(pos.x + dir.x * MOVESPEED) + int(pos.y) * MAPWIDTH] == false) pos.x += dir.x * MOVESPEED;
			if (worldMap[int(pos.x) + int(pos.y + dir.y * MOVESPEED) * MAPWIDTH] == false) pos.y += dir.y * MOVESPEED;
		}
		else if (GetInputPressed(GLFW_KEY_S))
		{
			if (worldMap[int(pos.x - dir.x * MOVESPEED) + int(pos.y) * MAPWIDTH] == false) pos.x -= dir.x * MOVESPEED;
			if (worldMap[int(pos.x) + int(pos.y - dir.y * MOVESPEED) * MAPWIDTH] == false) pos.y -= dir.y * MOVESPEED;
		}
		if (GetInputPressed(GLFW_KEY_A))
		{
			constexpr float rot = glm::pi<float>() / 2.f;

			glm::dmat2 rotation = { glm::dvec2(cos(rot), -sin(rot)), glm::dvec2(sin(rot), cos(rot)) };
			dir = dir * rotation;
			plane = plane * rotation;

			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x + dir.x * MOVESPEED) + int(pos.y) * MAPWIDTH] == false) pos.x += dir.x * MOVESPEED;
			if (worldMap[int(pos.x) + int(pos.y + dir.y * MOVESPEED) * MAPWIDTH] == false) pos.y += dir.y * MOVESPEED;

			// Rotate back to original state
			rotation = { glm::dvec2(cos(-rot), -sin(-rot)), glm::dvec2(sin(-rot), cos(-rot)) };
			dir = dir * rotation;
			plane = plane * rotation;
		}
		else if (GetInputPressed(GLFW_KEY_D))
		{
			constexpr float rot = -glm::pi<float>() / 2.f;

			glm::dmat2 rotation = { glm::dvec2(cos(rot), -sin(rot)), glm::dvec2(sin(rot), cos(rot)) };
			dir = dir * rotation;
			plane = plane * rotation;

			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x + dir.x * MOVESPEED) + int(pos.y) * MAPWIDTH] == false) pos.x += dir.x * MOVESPEED;
			if (worldMap[int(pos.x) + int(pos.y + dir.y * MOVESPEED) * MAPWIDTH] == false) pos.y += dir.y * MOVESPEED;

			// Rotate back to original state
			rotation = { glm::dvec2(cos(-rot), -sin(-rot)), glm::dvec2(sin(-rot), cos(-rot)) };
			dir = dir * rotation;
			plane = plane * rotation;
		}

		// Activate shader and render
		glUseProgram(shaderID);

		// Dynamic uniforms
		glUniform2f(uniform_pos, pos.x, pos.y);
		glUniform2f(uniform_dir, dir.x, dir.y);
		glUniform2f(uniform_plane, plane.x, plane.y);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Update
		glfwSwapBuffers(window);
		glfwPollEvents();

		// ========== TIMING ==========

		oldTime = time;
		time = glfwGetTime();
		frameTime = time - oldTime;
		//std::cout << frameTime << std::endl;
	}

	// ========== CLEAN UP ==========

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();

	return EXIT_SUCCESS;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouse_move.first)
	{
		mouse_move.last.x = xpos;
		mouse_move.last.y = ypos;
		mouse_move.first = false;
	}

	double x, y;
	glfwGetCursorPos(window, &x, &y);

	mouse_move.delta.x = xpos - mouse_move.last.x;
	mouse_move.delta.y = mouse_move.last.y - ypos; // reversed since y-coordinates go from bottom to top

	mouse_move.last.x = xpos;
	mouse_move.last.y = ypos;

	//std::cout << x << std::endl;
	//std::cout << mouse_move.delta.x << std::endl;

	double delta = -1.0 * mouse_move.delta.x * ROTSPEED  * frameTime;

	// Rotate dir and plane
	glm::dmat2 rotation = { glm::dvec2(cos(delta), -sin(delta)), glm::dvec2(sin(delta), cos(delta)) };
	dir = dir * rotation;
	plane = plane * rotation;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (action)
	{
	case GLFW_PRESS:
		inputs[key] |= INPUT_PRESSED;
		break;
	case GLFW_RELEASE:
		inputs[key] &= ~INPUT_PRESSED & ~INPUT_HELD;
		break;
	case GLFW_REPEAT:
		inputs[key] |= INPUT_HELD;
		break;
	default:
		break;
	}
}

void CompileShaders(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
	// 1. retrieve the vertex/fragment source code from filePath
	std::string vertexCode;
	std::string fragmentCode;
	std::string geometryCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	std::ifstream gShaderFile;
	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
		// if geometry shader path is present, also load a geometry shader
		if (geometryPath != nullptr)
		{
			gShaderFile.open(geometryPath);
			std::stringstream gShaderStream;
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
			geometryCode = gShaderStream.str();
		}
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();
	// 2. compile shaders
	unsigned int vertex, fragment;
	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");
	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");
	// if geometry shader is given, compile geometry shader
	unsigned int geometry;
	if (geometryPath != nullptr)
	{
		const char* gShaderCode = geometryCode.c_str();
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);
		checkCompileErrors(geometry, "GEOMETRY");
	}
	// shader Program
	shaderID = glCreateProgram();
	glAttachShader(shaderID, vertex);
	glAttachShader(shaderID, fragment);
	if (geometryPath != nullptr)
		glAttachShader(shaderID, geometry);
	glLinkProgram(shaderID);
	checkCompileErrors(shaderID, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessery
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	if (geometryPath != nullptr)
		glDeleteShader(geometry);
}

void checkCompileErrors(GLuint shader, std::string type)
{
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
}