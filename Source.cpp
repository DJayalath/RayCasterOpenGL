#include <glad/glad.h>
#include <SDL2/SDL.h>

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

#define NUM_TEXTURES 11
#define TEX_WIDTH 64
#define TEX_HEIGHT 64

#define MOVESPEED 0.02
#define ROTSPEED 0.10

SDL_Window* window = nullptr;
SDL_GLContext glContext;
SDL_Event event;
#define NUM_KEYS 128
#define NUM_MOUSE 5
bool m_keys[NUM_KEYS], m_mouse[NUM_MOUSE];
float frameTime;
double xlast;

glm::dvec2 pos, dir, plane;

struct Mouse
{
	glm::dvec2 last;
	glm::dvec2 delta;
	bool first = true;
} mouse_move;

unsigned int shaderID;

void CompileShaders(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
void checkCompileErrors(GLuint shader, std::string type);

int worldMap[MAPWIDTH * MAPHEIGHT] =
{
  8,8,8,8,8,8,8,8,8,8,8,4,4,6,4,4,6,4,6,4,4,4,6,4,
  8,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,0,0,0,0,0,0,0,4,
  8,0,3,3,0,0,0,0,0,8,8,4,0,0,0,0,0,0,0,0,0,0,0,6,
  8,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,
  8,0,3,3,0,0,0,0,0,8,8,4,0,0,0,0,0,0,0,0,0,0,0,4,
  8,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,0,6,6,6,0,6,4,6,
  8,8,8,8,0,8,8,8,8,8,8,4,4,4,4,4,4,6,0,0,0,0,0,6,
  7,7,7,7,0,7,7,7,7,0,8,0,8,0,8,0,8,4,0,4,0,6,0,6,
  7,7,0,0,0,0,0,0,7,8,0,8,0,8,0,8,8,6,0,0,0,0,0,6,
  7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,6,0,0,0,0,0,4,
  7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,6,0,6,0,6,0,6,
  7,7,0,0,0,0,0,0,7,8,0,8,0,8,0,8,8,6,4,6,0,6,6,6,
  7,7,7,7,0,7,7,7,7,8,8,4,0,6,8,4,8,3,3,3,0,3,3,3,
  2,2,2,2,0,2,2,2,2,4,6,4,0,0,6,0,6,3,0,0,0,0,0,3,
  2,2,0,0,0,0,0,2,2,4,0,0,0,0,0,0,4,3,0,0,0,0,0,3,
  2,0,0,0,0,0,0,0,2,4,0,0,0,0,0,0,4,3,0,0,0,0,0,3,
  1,0,0,0,0,0,0,0,1,4,4,4,4,4,6,0,6,3,3,0,0,0,3,3,
  2,0,0,0,0,0,0,0,2,2,2,1,2,2,2,6,6,0,0,5,0,5,0,5,
  2,2,0,0,0,0,0,2,2,2,0,0,0,2,2,0,5,0,5,0,0,0,5,5,
  2,0,0,0,0,0,0,0,2,0,0,0,0,0,2,5,0,5,0,5,0,5,0,5,
  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,
  2,0,0,0,0,0,0,0,2,0,0,0,0,0,2,5,0,5,0,5,0,5,0,5,
  2,2,0,0,0,0,0,2,2,2,0,0,0,2,2,0,5,0,5,0,0,0,5,5,
  2,2,2,2,1,2,2,2,2,2,2,1,2,2,2,5,5,5,5,5,5,5,5,5
};

struct Sprite
{
	double x;
	double y;
	int texture;

	int uDiv = 1, vDiv = 1;
	double vMove = 0.0;
};

#define numSprites 19

Sprite sprite[numSprites] =
{
  {20.5, 11.5, 10}, //green light in front of playerstart
  //green lights in every room
  {18.5,4.5, 10},
  {10.0,4.5, 10},
  {10.0,12.5,10},
  {3.5, 6.5, 10},
  {3.5, 20.5,10},
  {3.5, 14.5,10},
  {14.5,20.5,10},

  //row of pillars in front of wall: fisheye test
  {18.5, 10.5, 9},
  {18.5, 11.5, 9},
  {18.5, 12.5, 9},

  //some barrels around the map
  {21.5, 1.5, 8},
  {15.5, 1.5, 8},
  {16.0, 1.8, 8},
  {16.2, 1.2, 8},
  {3.5,  2.5, 8},
  {9.5, 15.5, 8},
  {10.0, 15.1,8},
  {10.5, 15.8,8},
};

//1D Zbuffer
double ZBuffer[W_WIDTH];

//arrays used to sort the sprites
int spriteOrder[numSprites];
double spriteDistance[numSprites];

//function used to sort the sprites
void combSort(int* order, double* dist, int amount);

int main(int argc, char* argv[])
{

	// ========== SDL2 BOILERPLATE ==========

	// Initialisation
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Create window
	window = SDL_CreateWindow("Ray Caster OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W_WIDTH, W_HEIGHT, SDL_WINDOW_OPENGL);
	glContext = SDL_GL_CreateContext(window);

	// Capture mouse
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// GLAD: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
		std::cout << "Failed to initialize GLAD" << std::endl;

	atexit(SDL_Quit);

	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// ========== RAY CASTING SETUP ==========

	pos = { 22, 13.5 };
	dir = { -1, 0 };
	plane = { 0, 0.66 };

	double time = 0; //time of current frame
	double oldTime = 0; //time of previous frame

	uint32_t px_buffer[W_WIDTH * W_HEIGHT];
	uint32_t texture[NUM_TEXTURES * TEX_WIDTH * TEX_HEIGHT];

	// load some textures
	std::string texture_locs[NUM_TEXTURES] =
	{
		"pics/eagle.png", "pics/redbrick.png", "pics/purplestone.png",
		"pics/greystone.png", "pics/bluestone.png", "pics/mossy.png",
		"pics/wood.png", "pics/colorstone.png", "pics/barrel.png",
		"pics/pillar.png", "pics/greenlight.png"
	};

	for (int i = 0; i < NUM_TEXTURES; i++)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);

		unsigned char* image = stbi_load
		(texture_locs[i].c_str(),
			&width,
			&height,
			&channels,
			STBI_rgb_alpha);

		memcpy(&texture[i * TEX_WIDTH * TEX_HEIGHT], image, sizeof(uint32_t) * width * height);
		stbi_image_free(image);
	}

	// ========== SHADER COMPILATION ==========

	// Compile
	CompileShaders("./shader.vert", "./shader.frag");

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

	unsigned int VBO, VAO, EBO, SSBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenBuffers(1, &SSBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Add texture data to SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(px_buffer), px_buffer, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, SSBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindVertexArray(0);


	// ========== GAME LOOP ==========

	bool quit = false;
	while (!quit)
	{
		// Clear previous buffer
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		// ========== RAY CASTING ==========
		// Ray cast for every vertical line of pixels in window
		for (int x = 0; x < W_WIDTH; x++)
		{
			// Map window coordinates to camera plane such that right = 1, left = -1
			double cameraX = (x / double(W_WIDTH)) * 2 - 1;

			// Ray direcrtion = direction vector + fraction of camera plane it passes through
			glm::dvec2 rayDir = dir + plane * cameraX;

			// DDA variables

			// which box of the map we're in
			glm::ivec2 map = static_cast<glm::ivec2>(pos);

			//length of ray from current position to next x or y-side
			glm::dvec2 sideDist;

			//length of ray from one x or y-side to next x or y-side
			glm::dvec2 deltaDist = glm::abs(1.0 / rayDir); // PROBLEM?
			double perpWallDist;

			//what direction to step in x or y-direction (either +1 or -1)
			glm::ivec2 step;

			bool hit = false; //was there a wall hit?
			int side; //was a NS or a EW wall hit?

			// Setup DDA, calc actual distances

			if (rayDir.x < 0)
			{
				step.x = -1;
				sideDist.x = (pos.x - map.x) * deltaDist.x;
			}
			else
			{
				step.x = 1;
				sideDist.x = (map.x + 1.0 - pos.x) * deltaDist.x;
			}
			if (rayDir.y < 0)
			{
				step.y = -1;
				sideDist.y = (pos.y - map.y) * deltaDist.y;
			}
			else
			{
				step.y = 1;
				sideDist.y = (map.y + 1.0 - pos.y) * deltaDist.y;
			}

			// Run DDA
			while (!hit)
			{
				//jump to next map square, OR in x-direction, OR in y-direction
				if (sideDist.x < sideDist.y)
				{
					sideDist.x += deltaDist.x;
					map.x += step.x;
					side = 0;
				}
				else
				{
					sideDist.y += deltaDist.y;
					map.y += step.y;
					side = 1;
				}
				//Check if ray has hit a wall
				if (worldMap[map.y * MAPWIDTH + map.x] > 0) hit = true;
			}

			//Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
			if (side == 0) perpWallDist = (map.x - pos.x + (1 - step.x) / 2) / rayDir.x;
			else           perpWallDist = (map.y - pos.y + (1 - step.y) / 2) / rayDir.y;

			if (perpWallDist < 0.0001f)
				perpWallDist = 0.0001f;

			// Calculate height of line to draw on screen
			// Also determines wall height (h * a) where a is height multiplier
			int lineHeight = (int)(W_HEIGHT / perpWallDist);

			// Limit lineHeight to prevent integer overflow
			if (lineHeight > 0xFFFF)
				lineHeight = 0xFFFF;

			//calculate lowest and highest pixel to fill in current stripe
			int drawStart = -lineHeight / 2 + W_HEIGHT / 2;
			if (drawStart < 0) drawStart = 0;
			int drawEnd = lineHeight / 2 + W_HEIGHT / 2;
			if (drawEnd >= W_HEIGHT) drawEnd = W_HEIGHT - 1;

			//texturing calculations
			int texNum = worldMap[map.y * MAPWIDTH + map.x] - 1; //1 subtracted from it so that texture 0 can be used!

			//calculate value of wallX
			double wallX; //where exactly the wall was hit
			if (side == 0) wallX = pos.y + perpWallDist * rayDir.y;
			else           wallX = pos.x + perpWallDist * rayDir.x;
			wallX -= floor((wallX));

			//x coordinate on the texture
			int texX = int(wallX * double(TEX_WIDTH));
			if (side == 0 && rayDir.x > 0) texX = TEX_WIDTH - texX - 1;
			if (side == 1 && rayDir.y < 0) texX = TEX_WIDTH - texX - 1;

			float inv_lineHeight = 1.f / lineHeight;
			for (int y = drawStart; y < drawEnd; y++)
			{
				int d = y * 256 - W_HEIGHT * 128 + lineHeight * 128;  //256 and 128 factors to avoid floats
				// TODO: avoid the division to speed this up
				int texY = int(((d * TEX_HEIGHT) * inv_lineHeight)) >> 8;
				Uint32 color = texture[texNum * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * texY + texX];
				//make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
				if (side == 1) color = (color >> 1) & 8355711;
				px_buffer[y * (W_WIDTH) + x] = color;
			}

			//SET THE ZBUFFER FOR SPRITE CASTING
			ZBuffer[x] = perpWallDist; //perpendicular distance is used

			//FLOOR CASTING
			double floorXWall, floorYWall; //x, y position of the floor texel at the bottom of the wall

			//4 different wall directions possible
			if (side == 0 && rayDir.x > 0)
			{
				floorXWall = map.x;
				floorYWall = map.y + wallX;
			}
			else if (side == 0 && rayDir.x < 0)
			{
				floorXWall = map.x + 1.0;
				floorYWall = map.y + wallX;
			}
			else if (side == 1 && rayDir.y > 0)
			{
				floorXWall = map.x + wallX;
				floorYWall = map.y;
			}
			else
			{
				floorXWall = map.x + wallX;
				floorYWall = map.y + 1.0;
			}

			double distWall, distPlayer, currentDist;

			distWall = perpWallDist;
			distPlayer = 0.0;

			if (drawEnd < 0) drawEnd = W_HEIGHT; //becomes < 0 when the integer overflows

			//draw the floor from drawEnd to the bottom of the screen
			for (int y = drawEnd + 1; y < W_HEIGHT; y++)
			{
				currentDist = W_HEIGHT / (2.0 * y - W_HEIGHT); //you could make a small lookup table for this instead

				double weight = (currentDist - distPlayer) / (distWall - distPlayer);

				double currentFloorX = weight * floorXWall + (1.0 - weight) * pos.x;
				double currentFloorY = weight * floorYWall + (1.0 - weight) * pos.y;

				// Division by 4 to scale texture for speed;
				int floorTexX, floorTexY;
				floorTexX = int(currentFloorX * TEX_WIDTH / 4) % TEX_WIDTH;
				floorTexY = int(currentFloorY * TEX_HEIGHT / 4) % TEX_HEIGHT;

				//floor
				Uint32 c = (texture[6 * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * floorTexY + floorTexX] >> 1) & 8355711;
				px_buffer[y * (W_WIDTH) + x] = c;
				//ceiling (symmetrical!)
				Uint32 c2 = texture[3 * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * floorTexY + floorTexX];
				px_buffer[(W_HEIGHT - y) * (W_WIDTH) + x] = c2;
			}
		}

		// Process inputs
		while (SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (event.key.keysym.sym < 128)
					m_keys[event.key.keysym.sym] = true;
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym < 128)
					m_keys[event.key.keysym.sym] = false;
				break;
			case SDL_MOUSEMOTION:

			{
				double delta = -1 * double(event.motion.xrel) * frameTime * ROTSPEED;

				// Rotate 90 degrees
				glm::dmat2 rotation = { glm::dvec2(cos(delta), -sin(delta)), glm::dvec2(sin(delta), cos(delta)) };
				dir = dir * rotation;
				plane = plane * rotation;
			}
			break;
			case SDL_QUIT:
				quit = true;
				break;
			default:
				break;
			}
		}

		if (m_keys[SDLK_ESCAPE])
			quit = true;

		// Normalize if simultaneous perpendicular inputs
		double multiplier = MOVESPEED * frameTime * 100.0;
		if (m_keys[SDLK_w] || m_keys[SDLK_s])
		{
			if (m_keys[SDLK_a] || m_keys[SDLK_d])
			{
				// Multiply by 1/sqrt(2) to normalize speeds when
				// moving in x and y directions simultaneously
				multiplier *= 0.70710678118;
			}
		}

		if (m_keys[SDLK_w])
		{
			if (worldMap[int(pos.x + dir.x * multiplier) + int(pos.y) * MAPWIDTH] == false) pos.x += dir.x * multiplier;
			if (worldMap[int(pos.x) + int(pos.y + dir.y * multiplier) * MAPWIDTH] == false) pos.y += dir.y * multiplier;
		}
		else if (m_keys[SDLK_s])
		{
			if (worldMap[int(pos.x - dir.x * multiplier) + int(pos.y) * MAPWIDTH] == false) pos.x -= dir.x * multiplier;
			if (worldMap[int(pos.x) + int(pos.y - dir.y * multiplier) * MAPWIDTH] == false) pos.y -= dir.y * multiplier;
		}
		if (m_keys[SDLK_a])
		{
			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x - dir.y * multiplier) + int(pos.y) * MAPWIDTH] == false) pos.x -= dir.y * multiplier;
			if (worldMap[int(pos.x) + int(pos.y + dir.x * multiplier) * MAPWIDTH] == false) pos.y += dir.x * multiplier;
		}
		else if (m_keys[SDLK_d])
		{
			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x + dir.y * multiplier) + int(pos.y) * MAPWIDTH] == false) pos.x += dir.y * multiplier;
			if (worldMap[int(pos.x) + int(pos.y - dir.x * multiplier) * MAPWIDTH] == false) pos.y -= dir.x * multiplier;
		}

		// Activate shader and render
		glUseProgram(shaderID);

		// Copy pixel buffer to shader
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(p, &px_buffer[0], sizeof(px_buffer));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Update
		SDL_GL_SwapWindow(window);

		// ========== TIMING ==========

		oldTime = time;
		time = SDL_GetTicks();
		frameTime = (time - oldTime) / 1000.0;
		std::cout << 1.0 / frameTime << std::endl;
	}

	// ========== CLEAN UP ==========

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
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