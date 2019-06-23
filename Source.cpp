#include <iostream>
#include <fstream>
#include <vector>

#include <SDL/SDL.h>
#include <SDL/SDL_render.h>
#include <glm/glm.hpp>

#define MAPWIDTH 24
#define MAPHEIGHT 24

#define WINDOWWIDTH 1280
#define WINDOWHEIGHT 720

#define TEXTUREWIDTH 64
#define TEXTUREHEIGHT 64

#define MOVESPEED 3.0
#define ROTSPEED 0.1

// Image loading functions
void loadFile(std::vector<unsigned char> & buffer, const std::string & filename);
int loadImage(std::vector<Uint32>& out, unsigned long& w, unsigned long& h, const std::string& filename);
int decodePNG(std::vector<unsigned char>& out_image, unsigned long& image_width, unsigned long& image_height, const unsigned char* in_png, size_t in_size, bool convert_to_rgba32 = true);
int decodePNG(std::vector<unsigned char>& out_image_32bit, unsigned long& image_width, unsigned long& image_height, const std::vector<unsigned char>& in_png);

int worldMap[MAPWIDTH][MAPHEIGHT] =
{
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7},
  {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,4,0,0,0,0,5,5,5,5,5,5,5,5,5,7,7,0,7,7,7,7,7},
  {4,0,5,0,0,0,0,5,0,5,0,5,0,5,0,5,7,0,0,0,7,7,7,1},
  {4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,7,1},
  {4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1},
  {4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1},
  {6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
  {6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2},
  {4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3}
};

#define NUM_KEYS 128
#define NUM_MOUSE 5

bool m_keys[NUM_KEYS], m_mouse[NUM_MOUSE];

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

Uint8 px_buffer[WINDOWWIDTH * WINDOWHEIGHT * 4];
static std::vector<Uint32> texture[11];

//1D Zbuffer
double ZBuffer[WINDOWWIDTH];

//arrays used to sort the sprites
int spriteOrder[numSprites];
double spriteDistance[numSprites];

//function used to sort the sprites
void combSort(int* order, double* dist, int amount);

int main(int argc, char* argv[])
{
	for (int i = 0; i < 11; i++)
	{
		texture[i].resize(TEXTUREWIDTH * TEXTUREHEIGHT);
	}

	//load some textures
	unsigned long tw, th, error = 0;
	error |= loadImage(texture[0], tw, th, "pics/eagle.png");
	error |= loadImage(texture[1], tw, th, "pics/redbrick.png");
	error |= loadImage(texture[2], tw, th, "pics/purplestone.png");
	error |= loadImage(texture[3], tw, th, "pics/greystone.png");
	error |= loadImage(texture[4], tw, th, "pics/bluestone.png");
	error |= loadImage(texture[5], tw, th, "pics/mossy.png");
	error |= loadImage(texture[6], tw, th, "pics/wood.png");
	error |= loadImage(texture[7], tw, th, "pics/colorstone.png");

	//load some sprite textures
	error |= loadImage(texture[8], tw, th, "pics/barrel.png");
	error |= loadImage(texture[9], tw, th, "pics/pillar.png");
	error |= loadImage(texture[10], tw, th, "pics/greenlight.png");

	// Test for error
	if (error) { std::cout << "error loading images" << std::endl; return 1; }

	SDL_Color c_random[4];

	for (int i = 0; i < 4; i++) c_random[i] = { Uint8(rand() * 255), Uint8(rand() * 255), Uint8(rand() * 255) };

	glm::dvec2 pos = { 22.0, 12.0 };
	glm::dvec2 dir = { -1.0, 0.0 };
	glm::dvec2 plane = { 0.0, 0.66 };

	double time = 0; // Time of current frame
	double oldTime = 0; // Time of previous frame

	// Stack size increased to 20MB
	SDL_Init(SDL_INIT_EVERYTHING);

	// Window
	SDL_Window* window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, WINDOWWIDTH, WINDOWHEIGHT, SDL_WINDOW_SHOWN);

	//SDL_Surface* surface = SDL_GetWindowSurface(window);

	SDL_Event e;

	SDL_Renderer* renderer = SDL_CreateRenderer
	(
		window,
		-1,
		SDL_RENDERER_ACCELERATED
	);

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	std::cout << "Renderer name: " << info.name << std::endl;
	std::cout << "Texture formats: " << std::endl;
	for (Uint32 i = 0; i < info.num_texture_formats; i++)
	{
		std::cout << SDL_GetPixelFormatName(info.texture_formats[i]) << std::endl;
	}

	SDL_Texture* sdl_texture = SDL_CreateTexture
	(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOWWIDTH, WINDOWHEIGHT
	);

	// Capture mouse
	SDL_SetRelativeMouseMode(SDL_TRUE);

	bool quit = false;

	// Game loop
	while (!quit)
	{

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		// Ray cast for every vertical line of pixels in window
		for (int x = 0; x < WINDOWWIDTH; x++)
		{
			// Map window coordinates to camera plane such that right = 1, left = -1
			double cameraX = (x / double(WINDOWWIDTH)) * 2 - 1;

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
				if (worldMap[map.x][map.y] > 0) hit = true;
			}

			//Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
			if (side == 0) perpWallDist = (map.x - pos.x + (1 - step.x) / 2) / rayDir.x;
			else           perpWallDist = (map.y - pos.y + (1 - step.y) / 2) / rayDir.y;

			if (perpWallDist < 0.0001f)
				perpWallDist = 0.0001f;

			// Calculate height of line to draw on screen
			// Also determines wall height (h * a) where a is height multiplier
			int lineHeight = (int)(WINDOWHEIGHT / perpWallDist);

			// Limit lineHeight to prevent integer overflow
			if (lineHeight > 0xFFFF)
				lineHeight = 0xFFFF;

			//calculate lowest and highest pixel to fill in current stripe
			int drawStart = -lineHeight / 2 + WINDOWHEIGHT / 2;
			if (drawStart < 0) drawStart = 0;
			int drawEnd = lineHeight / 2 + WINDOWHEIGHT / 2;
			if (drawEnd >= WINDOWHEIGHT) drawEnd = WINDOWHEIGHT - 1;

			//texturing calculations
			int texNum = worldMap[map.x][map.y] - 1; //1 subtracted from it so that texture 0 can be used!

			//calculate value of wallX
			double wallX; //where exactly the wall was hit
			if (side == 0) wallX = pos.y + perpWallDist * rayDir.y;
			else           wallX = pos.x + perpWallDist * rayDir.x;
			wallX -= floor((wallX));

			//x coordinate on the texture
			int texX = int(wallX * double(TEXTUREWIDTH));
			if (side == 0 && rayDir.x > 0) texX = TEXTUREWIDTH - texX - 1;
			if (side == 1 && rayDir.y < 0) texX = TEXTUREWIDTH - texX - 1;

			float inv_lineHeight = 1.f / lineHeight;
			for (int y = drawStart; y < drawEnd; y++)
			{
				int d = y * 256 - WINDOWHEIGHT * 128 + lineHeight * 128;  //256 and 128 factors to avoid floats
				// TODO: avoid the division to speed this up
				int texY = int(((d * TEXTUREHEIGHT) * inv_lineHeight)) >> 8;
				Uint32 color = texture[texNum][(int)TEXTUREHEIGHT * texY + texX];
				//make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
				if (side == 1) color = (color >> 1) & 8355711;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4] = color & 0x000000FF;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 1] = (color & 0x0000FF00) >> 8;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 2] = (color & 0x00FF0000) >> 16;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 3] = 0xFF;
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

			if (drawEnd < 0) drawEnd = WINDOWHEIGHT; //becomes < 0 when the integer overflows

			//draw the floor from drawEnd to the bottom of the screen
			for (int y = drawEnd + 1; y < WINDOWHEIGHT; y++)
			{
				currentDist = WINDOWHEIGHT / (2.0 * y - WINDOWHEIGHT); //you could make a small lookup table for this instead

				double weight = (currentDist - distPlayer) / (distWall - distPlayer);

				double currentFloorX = weight * floorXWall + (1.0 - weight) * pos.x;
				double currentFloorY = weight * floorYWall + (1.0 - weight) * pos.y;

				// Division by 4 to scale texture for speed;
				int floorTexX, floorTexY;
				floorTexX = int(currentFloorX * TEXTUREWIDTH / 4) % TEXTUREWIDTH;
				floorTexY = int(currentFloorY * TEXTUREHEIGHT / 4) % TEXTUREHEIGHT;

				//floor
				Uint32 c = (texture[3][(int)TEXTUREWIDTH * floorTexY + floorTexX] >> 1) & 8355711;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4] = c & 0x000000FF;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 1] = (c & 0x0000FF00) >> 8;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 2] = (c & 0x00FF0000) >> 16;
				px_buffer[y * (WINDOWWIDTH * 4) + x * 4 + 3] = 0xFF;
				//ceiling (symmetrical!)
				Uint32 c2 = texture[6][(int)TEXTUREWIDTH * floorTexY + floorTexX];
				px_buffer[(WINDOWHEIGHT - y) * (WINDOWWIDTH * 4) + x * 4] = c2 & 0x000000FF;
				px_buffer[(WINDOWHEIGHT - y) * (WINDOWWIDTH * 4) + x * 4 + 1] = (c2 & 0x0000FF00) >> 8;
				px_buffer[(WINDOWHEIGHT - y) * (WINDOWWIDTH * 4) + x * 4 + 2] = (c2 & 0x00FF0000) >> 16;
				px_buffer[(WINDOWHEIGHT - y) * (WINDOWWIDTH * 4) + x * 4 + 3] = 0xFF;
			}
		}

		//SPRITE CASTING
		//sort sprites from far to close
		for (int i = 0; i < numSprites; i++)
		{
			spriteOrder[i] = i;
			spriteDistance[i] = ((pos.x - sprite[i].x) * (pos.x - sprite[i].x) + (pos.y - sprite[i].y) * (pos.y - sprite[i].y)); //sqrt not taken, unneeded
		}
		combSort(spriteOrder, spriteDistance, numSprites);

		//after sorting the sprites, do the projection and draw them
		for (int i = 0; i < numSprites; i++)
		{
			//translate sprite position to relative to camera
			double spriteX = sprite[spriteOrder[i]].x - pos.x;
			double spriteY = sprite[spriteOrder[i]].y - pos.y;

			//transform sprite with the inverse camera matrix
			// [ plane.x   dir.x ] -1                                       [ dir.y      -dir.x ]
			// [               ]       =  1/(plane.x*dir.y-dir.x*plane.y) *   [                 ]
			// [ plane.y   dir.y ]                                          [ -plane.y  plane.x ]

			double invDet = 1.0 / (plane.x * dir.y - dir.x * plane.y); //required for correct matrix multiplication

			double transformX = invDet * (dir.y * spriteX - dir.x * spriteY);
			double transformY = invDet * (-plane.y * spriteX + plane.x * spriteY); //this is actually the depth inside the screen, that what Z is in 3D

			int spriteScreenX = int((WINDOWWIDTH / 2) * (1 + transformX / transformY));

			// Moving sprite variables
			int vMoveScreen = int(sprite[spriteOrder[i]].vMove / transformY);

			//calculate height of the sprite on screen
			int spriteHeight = abs(int(WINDOWHEIGHT / (transformY))) / sprite[spriteOrder[i]].vDiv; //using "transformY" instead of the real distance prevents fisheye
			// Limit to prevent integer overflow
			if (spriteHeight > 0xFFFF)
				spriteHeight = 0xFFFF;
			//calculate lowest and highest pixel to fill in current stripe
			int drawStartY = -spriteHeight / 2 + WINDOWHEIGHT / 2 + vMoveScreen;
			if (drawStartY < 0) drawStartY = 0;
			int drawEndY = spriteHeight / 2 + WINDOWHEIGHT / 2 + vMoveScreen;
			if (drawEndY >= WINDOWHEIGHT) drawEndY = WINDOWHEIGHT - 1;

			//calculate width of the sprite
			int spriteWidth = abs(int(WINDOWHEIGHT / (transformY))) / sprite[spriteOrder[i]].uDiv;
			// Limit to prevent integer overflow
			if (spriteWidth > 0xFFFF)
				spriteWidth = 0xFFFF;
			int drawStartX = -spriteWidth / 2 + spriteScreenX;
			if (drawStartX < 0) drawStartX = 0;
			int drawEndX = spriteWidth / 2 + spriteScreenX;
			if (drawEndX >= WINDOWWIDTH) drawEndX = WINDOWWIDTH - 1;

			//loop through every vertical stripe of the sprite on screen
			for (int stripe = drawStartX; stripe < drawEndX; stripe++)
			{
				int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * TEXTUREWIDTH / spriteWidth) / 256;
				//the conditions in the if are:
				//1) it's in front of camera plane so you don't see things behind you
				//2) it's on the screen (left)
				//3) it's on the screen (right)
				//4) ZBuffer, with perpendicular distance
				if (transformY > 0 && stripe > 0 && stripe < WINDOWWIDTH && transformY < ZBuffer[stripe])
					for (int y = drawStartY; y < drawEndY; y++) //for every pixel of the current stripe
					{
						int d = (y - vMoveScreen) * 256 - WINDOWHEIGHT * 128 + spriteHeight * 128; //256 and 128 factors to avoid floats
						int texY = ((d * TEXTUREHEIGHT) / spriteHeight) / 256;
						Uint32 color = texture[sprite[spriteOrder[i]].texture][(int)TEXTUREWIDTH * texY + texX]; //get current color from the texture
						//paint pixel if it isn't black, black is the invisible color
						if ((color & 0x00FFFFFF) != 0)
						{
							px_buffer[y * (WINDOWWIDTH * 4) + stripe * 4] = color & 0x000000FF;
							px_buffer[y * (WINDOWWIDTH * 4) + stripe * 4 + 1] = (color & 0x0000FF00) >> 8;
							px_buffer[y * (WINDOWWIDTH * 4) + stripe * 4 + 2] = (color & 0x00FF0000) >> 16;
							px_buffer[y * (WINDOWWIDTH * 4) + stripe * 4 + 3] = 0xFF;
						}

					}
			}
		}

		SDL_UpdateTexture
		(
			sdl_texture,
			NULL,
			&px_buffer[0],
			WINDOWWIDTH * 4
		);

		SDL_RenderCopy(renderer, sdl_texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		//timing for input and FPS counter

		oldTime = time;
		time = SDL_GetTicks();
		double frameTime = (time - oldTime) / 1000.0; //frameTime is the time this frame has taken, in seconds

		//std::cout << 1.0 / frameTime << std::endl; //FPS counter

		// Speed modifiers
		double moveSpeed = frameTime * MOVESPEED; //the constant value is in squares/second

		// Key polling
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_KEYDOWN:
				if (e.key.keysym.sym < 128)
					m_keys[e.key.keysym.sym] = true;
				break;
			case SDL_KEYUP:
				if (e.key.keysym.sym < 128)
					m_keys[e.key.keysym.sym] = false;
				break;
			case SDL_MOUSEMOTION:

			{
				double delta = -1 * double(e.motion.xrel) * frameTime * ROTSPEED;

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

		if (m_keys[SDLK_w] || m_keys[SDLK_s])
		{
			if (m_keys[SDLK_a] || m_keys[SDLK_d])
			{
				// Multiply by 1/sqrt(2) to normalize speeds when
				// moving in x and y directions simultaneously
				moveSpeed *= 0.70710678118;
			}
		}

		// Simple movement, checking if wall present
		if (m_keys[SDLK_w])
		{
			if (worldMap[int(pos.x + dir.x * moveSpeed)][int(pos.y)] == false) pos.x += dir.x * moveSpeed;
			if (worldMap[int(pos.x)][int(pos.y + dir.y * moveSpeed)] == false) pos.y += dir.y * moveSpeed;
		}
		if (m_keys[SDLK_s])
		{
			if (worldMap[int(pos.x - dir.x * moveSpeed)][int(pos.y)] == false) pos.x -= dir.x * moveSpeed;
			if (worldMap[int(pos.x)][int(pos.y - dir.y * moveSpeed)] == false) pos.y -= dir.y * moveSpeed;
		}

		// Rotation using 2D rotation matrix
		if (m_keys[SDLK_d])
		{
			double rot = -M_PI / 2;

			glm::dmat2 rotation = { glm::dvec2(cos(rot), -sin(rot)), glm::dvec2(sin(rot), cos(rot)) };
			dir = dir * rotation;
			plane = plane * rotation;

			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x + dir.x * moveSpeed)][int(pos.y)] == false) pos.x += dir.x * moveSpeed;
			if (worldMap[int(pos.x)][int(pos.y + dir.y * moveSpeed)] == false) pos.y += dir.y * moveSpeed;

			// Rotate back to original state
			rotation = { glm::dvec2(cos(-rot), -sin(-rot)), glm::dvec2(sin(-rot), cos(-rot)) };
			dir = dir * rotation;
			plane = plane * rotation;
		}
		if (m_keys[SDLK_a])
		{
			double rot = M_PI / 2;

			glm::dmat2 rotation = { glm::dvec2(cos(rot), -sin(rot)), glm::dvec2(sin(rot), cos(rot)) };
			dir = dir * rotation;
			plane = plane * rotation;

			// Apply transformation as if moving forwards
			if (worldMap[int(pos.x + dir.x * moveSpeed)][int(pos.y)] == false) pos.x += dir.x * moveSpeed;
			if (worldMap[int(pos.x)][int(pos.y + dir.y * moveSpeed)] == false) pos.y += dir.y * moveSpeed;

			// Rotate back to original state
			rotation = { glm::dvec2(cos(-rot), -sin(-rot)), glm::dvec2(sin(-rot), cos(-rot)) };
			dir = dir * rotation;
			plane = plane * rotation;
		}
	}

	// ========== Cleanup ==========
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
};













//sort algorithm
void combSort(int* order, double* dist, int amount)
{
	int gap = amount;
	bool swapped = false;
	while (gap > 1 || swapped)
	{
		//shrink factor 1.3
		gap = (gap * 10) / 13;
		if (gap == 9 || gap == 10) gap = 11;
		if (gap < 1) gap = 1;
		swapped = false;
		for (int i = 0; i < amount - gap; i++)
		{
			int j = i + gap;
			if (dist[i] < dist[j])
			{
				std::swap(dist[i], dist[j]);
				std::swap(order[i], order[j]);
				swapped = true;
			}
		}
	}
}

void loadFile(std::vector<unsigned char>& buffer, const std::string& filename) //designed for loading files from hard disk in an std::vector
{
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

	//get filesize
	std::streamsize size = 0;
	if (file.seekg(0, std::ios::end).good()) size = file.tellg();
	if (file.seekg(0, std::ios::beg).good()) size -= file.tellg();

	//read contents of the file into the vector
	buffer.resize(size_t(size));
	if (size > 0) file.read((char*)(&buffer[0]), size);
}

int loadImage(std::vector<Uint32>& out, unsigned long& w, unsigned long& h, const std::string& filename)
{
	std::vector<unsigned char> file, image;
	loadFile(file, filename);
	if (decodePNG(image, w, h, file)) return 1;

	out.resize(image.size() / 4);

	for (size_t i = 0; i < out.size(); i++)
	{
		out[i] = 0x1000000 * image[i * 4 + 3] + 0x10000 * image[i * 4 + 0] + 0x100 * image[i * 4 + 1] + image[i * 4 + 2];
	}

	return 0;
}

int decodePNG(std::vector<unsigned char>& out_image, unsigned long& image_width, unsigned long& image_height, const unsigned char* in_png, size_t in_size, bool convert_to_rgba32)
{
	// picoPNG version 20101224
	// Copyright (c) 2005-2010 Lode Vandevenne
	//
	// This software is provided 'as-is', without any express or implied
	// warranty. In no event will the authors be held liable for any damages
	// arising from the use of this software.
	//
	// Permission is granted to anyone to use this software for any purpose,
	// including commercial applications, and to alter it and redistribute it
	// freely, subject to the following restrictions:
	//
	//     1. The origin of this software must not be misrepresented; you must not
	//     claim that you wrote the original software. If you use this software
	//     in a product, an acknowledgment in the product documentation would be
	//     appreciated but is not required.
	//     2. Altered source versions must be plainly marked as such, and must not be
	//     misrepresented as being the original software.
	//     3. This notice may not be removed or altered from any source distribution.

	static const unsigned long LENBASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
	static const unsigned long LENEXTRA[29] = { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
	static const unsigned long DISTBASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
	static const unsigned long DISTEXTRA[30] = { 0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13 };
	static const unsigned long CLCL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 }; //code length code lengths
	struct Zlib //nested functions for zlib decompression
	{
		static unsigned long readBitFromStream(size_t& bitp, const unsigned char* bits) { unsigned long result = (bits[bitp >> 3] >> (bitp & 0x7)) & 1; bitp++; return result; }
		static unsigned long readBitsFromStream(size_t& bitp, const unsigned char* bits, size_t nbits)
		{
			unsigned long result = 0;
			for (size_t i = 0; i < nbits; i++) result += (readBitFromStream(bitp, bits)) << i;
			return result;
		}
		struct HuffmanTree
		{
			int makeFromLengths(const std::vector<unsigned long>& bitlen, unsigned long maxbitlen)
			{ //make tree given the lengths
				unsigned long numcodes = (unsigned long)(bitlen.size()), treepos = 0, nodefilled = 0;
				std::vector<unsigned long> tree1d(numcodes), blcount(maxbitlen + 1, 0), nextcode(maxbitlen + 1, 0);
				for (unsigned long bits = 0; bits < numcodes; bits++) blcount[bitlen[bits]]++; //count number of instances of each code length
				for (unsigned long bits = 1; bits <= maxbitlen; bits++) nextcode[bits] = (nextcode[bits - 1] + blcount[bits - 1]) << 1;
				for (unsigned long n = 0; n < numcodes; n++) if (bitlen[n] != 0) tree1d[n] = nextcode[bitlen[n]]++; //generate all the codes
				tree2d.clear(); tree2d.resize(numcodes * 2, 32767); //32767 here means the tree2d isn't filled there yet
				for (unsigned long n = 0; n < numcodes; n++) //the codes
					for (unsigned long i = 0; i < bitlen[n]; i++) //the bits for this code
					{
						unsigned long bit = (tree1d[n] >> (bitlen[n] - i - 1)) & 1;
						if (treepos > numcodes - 2) return 55;
						if (tree2d[2 * treepos + bit] == 32767) //not yet filled in
						{
							if (i + 1 == bitlen[n]) { tree2d[2 * treepos + bit] = n; treepos = 0; } //last bit
							else { tree2d[2 * treepos + bit] = ++nodefilled + numcodes; treepos = nodefilled; } //addresses are encoded as values > numcodes
						}
						else treepos = tree2d[2 * treepos + bit] - numcodes; //subtract numcodes from address to get address value
					}
				return 0;
			}
			int decode(bool& decoded, unsigned long& result, size_t& treepos, unsigned long bit) const
			{ //Decodes a symbol from the tree
				unsigned long numcodes = (unsigned long)tree2d.size() / 2;
				if (treepos >= numcodes) return 11; //error: you appeared outside the codetree
				result = tree2d[2 * treepos + bit];
				decoded = (result < numcodes);
				treepos = decoded ? 0 : result - numcodes;
				return 0;
			}
			std::vector<unsigned long> tree2d; //2D representation of a huffman tree: The one dimension is "0" or "1", the other contains all nodes and leaves of the tree.
		};
		struct Inflator
		{
			int error;
			void inflate(std::vector<unsigned char>& out, const std::vector<unsigned char>& in, size_t inpos = 0)
			{
				size_t bp = 0, pos = 0; //bit pointer and byte pointer
				error = 0;
				unsigned long BFINAL = 0;
				while (!BFINAL && !error)
				{
					if (bp >> 3 >= in.size()) { error = 52; return; } //error, bit pointer will jump past memory
					BFINAL = readBitFromStream(bp, &in[inpos]);
					unsigned long BTYPE = readBitFromStream(bp, &in[inpos]); BTYPE += 2 * readBitFromStream(bp, &in[inpos]);
					if (BTYPE == 3) { error = 20; return; } //error: invalid BTYPE
					else if (BTYPE == 0) inflateNoCompression(out, &in[inpos], bp, pos, in.size());
					else inflateHuffmanBlock(out, &in[inpos], bp, pos, in.size(), BTYPE);
				}
				if (!error) out.resize(pos); //Only now we know the true size of out, resize it to that
			}
			void generateFixedTrees(HuffmanTree& tree, HuffmanTree& treeD) //get the tree of a deflated block with fixed tree
			{
				std::vector<unsigned long> bitlen(288, 8), bitlenD(32, 5);;
				for (size_t i = 144; i <= 255; i++) bitlen[i] = 9;
				for (size_t i = 256; i <= 279; i++) bitlen[i] = 7;
				tree.makeFromLengths(bitlen, 15);
				treeD.makeFromLengths(bitlenD, 15);
			}
			HuffmanTree codetree, codetreeD, codelengthcodetree; //the code tree for Huffman codes, dist codes, and code length codes
			unsigned long huffmanDecodeSymbol(const unsigned char* in, size_t& bp, const HuffmanTree& codetree, size_t inlength)
			{ //decode a single symbol from given list of bits with given code tree. return value is the symbol
				bool decoded; unsigned long ct;
				for (size_t treepos = 0;;)
				{
					if ((bp & 0x07) == 0 && (bp >> 3) > inlength) { error = 10; return 0; } //error: end reached without endcode
					error = codetree.decode(decoded, ct, treepos, readBitFromStream(bp, in)); if (error) return 0; //stop, an error happened
					if (decoded) return ct;
				}
			}
			void getTreeInflateDynamic(HuffmanTree& tree, HuffmanTree& treeD, const unsigned char* in, size_t& bp, size_t inlength)
			{ //get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree
				std::vector<unsigned long> bitlen(288, 0), bitlenD(32, 0);
				if (bp >> 3 >= inlength - 2) { error = 49; return; } //the bit pointer is or will go past the memory
				size_t HLIT = readBitsFromStream(bp, in, 5) + 257; //number of literal/length codes + 257
				size_t HDIST = readBitsFromStream(bp, in, 5) + 1; //number of dist codes + 1
				size_t HCLEN = readBitsFromStream(bp, in, 4) + 4; //number of code length codes + 4
				std::vector<unsigned long> codelengthcode(19); //lengths of tree to decode the lengths of the dynamic tree
				for (size_t i = 0; i < 19; i++) codelengthcode[CLCL[i]] = (i < HCLEN) ? readBitsFromStream(bp, in, 3) : 0;
				error = codelengthcodetree.makeFromLengths(codelengthcode, 7); if (error) return;
				size_t i = 0, replength;
				while (i < HLIT + HDIST)
				{
					unsigned long code = huffmanDecodeSymbol(in, bp, codelengthcodetree, inlength); if (error) return;
					if (code <= 15) { if (i < HLIT) bitlen[i++] = code; else bitlenD[i++ - HLIT] = code; } //a length code
					else if (code == 16) //repeat previous
					{
						if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
						replength = 3 + readBitsFromStream(bp, in, 2);
						unsigned long value; //set value to the previous code
						if ((i - 1) < HLIT) value = bitlen[i - 1];
						else value = bitlenD[i - HLIT - 1];
						for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
						{
							if (i >= HLIT + HDIST) { error = 13; return; } //error: i is larger than the amount of codes
							if (i < HLIT) bitlen[i++] = value; else bitlenD[i++ - HLIT] = value;
						}
					}
					else if (code == 17) //repeat "0" 3-10 times
					{
						if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
						replength = 3 + readBitsFromStream(bp, in, 3);
						for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
						{
							if (i >= HLIT + HDIST) { error = 14; return; } //error: i is larger than the amount of codes
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else if (code == 18) //repeat "0" 11-138 times
					{
						if (bp >> 3 >= inlength) { error = 50; return; } //error, bit pointer jumps past memory
						replength = 11 + readBitsFromStream(bp, in, 7);
						for (size_t n = 0; n < replength; n++) //repeat this value in the next lengths
						{
							if (i >= HLIT + HDIST) { error = 15; return; } //error: i is larger than the amount of codes
							if (i < HLIT) bitlen[i++] = 0; else bitlenD[i++ - HLIT] = 0;
						}
					}
					else { error = 16; return; } //error: somehow an unexisting code appeared. This can never happen.
				}
				if (bitlen[256] == 0) { error = 64; return; } //the length of the end code 256 must be larger than 0
				error = tree.makeFromLengths(bitlen, 15); if (error) return; //now we've finally got HLIT and HDIST, so generate the code trees, and the function is done
				error = treeD.makeFromLengths(bitlenD, 15); if (error) return;
			}
			void inflateHuffmanBlock(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength, unsigned long btype)
			{
				if (btype == 1) { generateFixedTrees(codetree, codetreeD); }
				else if (btype == 2) { getTreeInflateDynamic(codetree, codetreeD, in, bp, inlength); if (error) return; }
				for (;;)
				{
					unsigned long code = huffmanDecodeSymbol(in, bp, codetree, inlength); if (error) return;
					if (code == 256) return; //end code
					else if (code <= 255) //literal symbol
					{
						if (pos >= out.size()) out.resize((pos + 1) * 2); //reserve more room
						out[pos++] = (unsigned char)(code);
					}
					else if (code >= 257 && code <= 285) //length code
					{
						size_t length = LENBASE[code - 257], numextrabits = LENEXTRA[code - 257];
						if ((bp >> 3) >= inlength) { error = 51; return; } //error, bit pointer will jump past memory
						length += readBitsFromStream(bp, in, numextrabits);
						unsigned long codeD = huffmanDecodeSymbol(in, bp, codetreeD, inlength); if (error) return;
						if (codeD > 29) { error = 18; return; } //error: invalid dist code (30-31 are never used)
						unsigned long dist = DISTBASE[codeD], numextrabitsD = DISTEXTRA[codeD];
						if ((bp >> 3) >= inlength) { error = 51; return; } //error, bit pointer will jump past memory
						dist += readBitsFromStream(bp, in, numextrabitsD);
						size_t start = pos, back = start - dist; //backwards
						if (pos + length >= out.size()) out.resize((pos + length) * 2); //reserve more room
						for (size_t i = 0; i < length; i++) { out[pos++] = out[back++]; if (back >= start) back = start - dist; }
					}
				}
			}
			void inflateNoCompression(std::vector<unsigned char>& out, const unsigned char* in, size_t& bp, size_t& pos, size_t inlength)
			{
				while ((bp & 0x7) != 0) bp++; //go to first boundary of byte
				size_t p = bp / 8;
				if (p >= inlength - 4) { error = 52; return; } //error, bit pointer will jump past memory
				unsigned long LEN = in[p] + 256 * in[p + 1], NLEN = in[p + 2] + 256 * in[p + 3]; p += 4;
				if (LEN + NLEN != 65535) { error = 21; return; } //error: NLEN is not one's complement of LEN
				if (pos + LEN >= out.size()) out.resize(pos + LEN);
				if (p + LEN > inlength) { error = 23; return; } //error: reading outside of in buffer
				for (unsigned long n = 0; n < LEN; n++) out[pos++] = in[p++]; //read LEN bytes of literal data
				bp = p * 8;
			}
		};
		int decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in) //returns error value
		{
			Inflator inflator;
			if (in.size() < 2) { return 53; } //error, size of zlib data too small
			if ((in[0] * 256 + in[1]) % 31 != 0) { return 24; } //error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way
			unsigned long CM = in[0] & 15, CINFO = (in[0] >> 4) & 15, FDICT = (in[1] >> 5) & 1;
			if (CM != 8 || CINFO > 7) { return 25; } //error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec
			if (FDICT != 0) { return 26; } //error: the specification of PNG says about the zlib stream: "The additional flags shall not specify a preset dictionary."
			inflator.inflate(out, in, 2);
			return inflator.error; //note: adler32 checksum was skipped and ignored
		}
	};
	struct PNG //nested functions for PNG decoding
	{
		struct Info
		{
			unsigned long width, height, colorType, bitDepth, compressionMethod, filterMethod, interlaceMethod, key_r, key_g, key_b;
			bool key_defined; //is a transparent color key given?
			std::vector<unsigned char> palette;
		} info;
		int error;
		void decode(std::vector<unsigned char>& out, const unsigned char* in, size_t size, bool convert_to_rgba32)
		{
			error = 0;
			if (size == 0 || in == 0) { error = 48; return; } //the given data is empty
			readPngHeader(&in[0], size); if (error) return;
			size_t pos = 33; //first byte of the first chunk after the header
			std::vector<unsigned char> idat; //the data from idat chunks
			bool IEND = false, known_type = true;
			info.key_defined = false;
			while (!IEND) //loop through the chunks, ignoring unknown chunks and stopping at IEND chunk. IDAT data is put at the start of the in buffer
			{
				if (pos + 8 >= size) { error = 30; return; } //error: size of the in buffer too small to contain next chunk
				size_t chunkLength = read32bitInt(&in[pos]); pos += 4;
				if (chunkLength > 2147483647) { error = 63; return; }
				if (pos + chunkLength >= size) { error = 35; return; } //error: size of the in buffer too small to contain next chunk
				if (in[pos + 0] == 'I' && in[pos + 1] == 'D' && in[pos + 2] == 'A' && in[pos + 3] == 'T') //IDAT chunk, containing compressed image data
				{
					idat.insert(idat.end(), &in[pos + 4], &in[pos + 4 + chunkLength]);
					pos += (4 + chunkLength);
				}
				else if (in[pos + 0] == 'I' && in[pos + 1] == 'E' && in[pos + 2] == 'N' && in[pos + 3] == 'D') { pos += 4; IEND = true; }
				else if (in[pos + 0] == 'P' && in[pos + 1] == 'L' && in[pos + 2] == 'T' && in[pos + 3] == 'E') //palette chunk (PLTE)
				{
					pos += 4; //go after the 4 letters
					info.palette.resize(4 * (chunkLength / 3));
					if (info.palette.size() > (4 * 256)) { error = 38; return; } //error: palette too big
					for (size_t i = 0; i < info.palette.size(); i += 4)
					{
						for (size_t j = 0; j < 3; j++) info.palette[i + j] = in[pos++]; //RGB
						info.palette[i + 3] = 255; //alpha
					}
				}
				else if (in[pos + 0] == 't' && in[pos + 1] == 'R' && in[pos + 2] == 'N' && in[pos + 3] == 'S') //palette transparency chunk (tRNS)
				{
					pos += 4; //go after the 4 letters
					if (info.colorType == 3)
					{
						if (4 * chunkLength > info.palette.size()) { error = 39; return; } //error: more alpha values given than there are palette entries
						for (size_t i = 0; i < chunkLength; i++) info.palette[4 * i + 3] = in[pos++];
					}
					else if (info.colorType == 0)
					{
						if (chunkLength != 2) { error = 40; return; } //error: this chunk must be 2 bytes for greyscale image
						info.key_defined = 1; info.key_r = info.key_g = info.key_b = 256 * in[pos] + in[pos + 1]; pos += 2;
					}
					else if (info.colorType == 2)
					{
						if (chunkLength != 6) { error = 41; return; } //error: this chunk must be 6 bytes for RGB image
						info.key_defined = 1;
						info.key_r = 256 * in[pos] + in[pos + 1]; pos += 2;
						info.key_g = 256 * in[pos] + in[pos + 1]; pos += 2;
						info.key_b = 256 * in[pos] + in[pos + 1]; pos += 2;
					}
					else { error = 42; return; } //error: tRNS chunk not allowed for other color models
				}
				else //it's not an implemented chunk type, so ignore it: skip over the data
				{
					if (!(in[pos + 0] & 32)) { error = 69; return; } //error: unknown critical chunk (5th bit of first byte of chunk type is 0)
					pos += (chunkLength + 4); //skip 4 letters and uninterpreted data of unimplemented chunk
					known_type = false;
				}
				pos += 4; //step over CRC (which is ignored)
			}
			unsigned long bpp = getBpp(info);
			std::vector<unsigned char> scanlines(((info.width * (info.height * bpp + 7)) / 8) + info.height); //now the out buffer will be filled
			Zlib zlib; //decompress with the Zlib decompressor
			error = zlib.decompress(scanlines, idat); if (error) return; //stop if the zlib decompressor returned an error
			size_t bytewidth = (bpp + 7) / 8, outlength = (info.height * info.width * bpp + 7) / 8;
			out.resize(outlength); //time to fill the out buffer
			unsigned char* out_ = outlength ? &out[0] : 0; //use a regular pointer to the std::vector for faster code if compiled without optimization
			if (info.interlaceMethod == 0) //no interlace, just filter
			{
				size_t linestart = 0, linelength = (info.width * bpp + 7) / 8; //length in bytes of a scanline, excluding the filtertype byte
				if (bpp >= 8) //byte per byte
					for (unsigned long y = 0; y < info.height; y++)
					{
						unsigned long filterType = scanlines[linestart];
						const unsigned char* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width * bytewidth];
						unFilterScanline(&out_[linestart - y], &scanlines[linestart + 1], prevline, bytewidth, filterType, linelength); if (error) return;
						linestart += (1 + linelength); //go to start of next scanline
					}
				else //less than 8 bits per pixel, so fill it up bit per bit
				{
					std::vector<unsigned char> templine((info.width * bpp + 7) >> 3); //only used if bpp < 8
					for (size_t y = 0, obp = 0; y < info.height; y++)
					{
						unsigned long filterType = scanlines[linestart];
						const unsigned char* prevline = (y == 0) ? 0 : &out_[(y - 1) * info.width * bytewidth];
						unFilterScanline(&templine[0], &scanlines[linestart + 1], prevline, bytewidth, filterType, linelength); if (error) return;
						for (size_t bp = 0; bp < info.width * bpp;) setBitOfReversedStream(obp, out_, readBitFromReversedStream(bp, &templine[0]));
						linestart += (1 + linelength); //go to start of next scanline
					}
				}
			}
			else //interlaceMethod is 1 (Adam7)
			{
				size_t passw[7] = { (info.width + 7) / 8, (info.width + 3) / 8, (info.width + 3) / 4, (info.width + 1) / 4, (info.width + 1) / 2, (info.width + 0) / 2, (info.width + 0) / 1 };
				size_t passh[7] = { (info.height + 7) / 8, (info.height + 7) / 8, (info.height + 3) / 8, (info.height + 3) / 4, (info.height + 1) / 4, (info.height + 1) / 2, (info.height + 0) / 2 };
				size_t passstart[7] = { 0 };
				size_t pattern[28] = { 0,4,0,2,0,1,0,0,0,4,0,2,0,1,8,8,4,4,2,2,1,8,8,8,4,4,2,2 }; //values for the adam7 passes
				for (int i = 0; i < 6; i++) passstart[i + 1] = passstart[i] + passh[i] * ((passw[i] ? 1 : 0) + (passw[i] * bpp + 7) / 8);
				std::vector<unsigned char> scanlineo((info.width * bpp + 7) / 8), scanlinen((info.width * bpp + 7) / 8); //"old" and "new" scanline
				for (int i = 0; i < 7; i++)
					adam7Pass(&out_[0], &scanlinen[0], &scanlineo[0], &scanlines[passstart[i]], info.width, pattern[i], pattern[i + 7], pattern[i + 14], pattern[i + 21], passw[i], passh[i], bpp);
			}
			if (convert_to_rgba32 && (info.colorType != 6 || info.bitDepth != 8)) //conversion needed
			{
				std::vector<unsigned char> data = out;
				error = convert(out, &data[0], info, info.width, info.height);
			}
		}
		void readPngHeader(const unsigned char* in, size_t inlength) //read the information from the header and store it in the Info
		{
			if (inlength < 29) { error = 27; return; } //error: the data length is smaller than the length of the header
			if (in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71 || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10) { error = 28; return; } //no PNG signature
			if (in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R') { error = 29; return; } //error: it doesn't start with a IHDR chunk!
			info.width = read32bitInt(&in[16]); info.height = read32bitInt(&in[20]);
			info.bitDepth = in[24]; info.colorType = in[25];
			info.compressionMethod = in[26]; if (in[26] != 0) { error = 32; return; } //error: only compression method 0 is allowed in the specification
			info.filterMethod = in[27]; if (in[27] != 0) { error = 33; return; } //error: only filter method 0 is allowed in the specification
			info.interlaceMethod = in[28]; if (in[28] > 1) { error = 34; return; } //error: only interlace methods 0 and 1 exist in the specification
			error = checkColorValidity(info.colorType, info.bitDepth);
		}
		void unFilterScanline(unsigned char* recon, const unsigned char* scanline, const unsigned char* precon, size_t bytewidth, unsigned long filterType, size_t length)
		{
			switch (filterType)
			{
			case 0: for (size_t i = 0; i < length; i++) recon[i] = scanline[i]; break;
			case 1:
				for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
				for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth];
				break;
			case 2:
				if (precon) for (size_t i = 0; i < length; i++) recon[i] = scanline[i] + precon[i];
				else       for (size_t i = 0; i < length; i++) recon[i] = scanline[i];
				break;
			case 3:
				if (precon)
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i] + precon[i] / 2;
					for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
				}
				else
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
					for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth] / 2;
				}
				break;
			case 4:
				if (precon)
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i] + paethPredictor(0, precon[i], 0);
					for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]);
				}
				else
				{
					for (size_t i = 0; i < bytewidth; i++) recon[i] = scanline[i];
					for (size_t i = bytewidth; i < length; i++) recon[i] = scanline[i] + paethPredictor(recon[i - bytewidth], 0, 0);
				}
				break;
			default: error = 36; return; //error: unexisting filter type given
			}
		}
		void adam7Pass(unsigned char* out, unsigned char* linen, unsigned char* lineo, const unsigned char* in, unsigned long w, size_t passleft, size_t passtop, size_t spacex, size_t spacey, size_t passw, size_t passh, unsigned long bpp)
		{ //filter and reposition the pixels into the output when the image is Adam7 interlaced. This function can only do it after the full image is already decoded. The out buffer must have the correct allocated memory size already.
			if (passw == 0) return;
			size_t bytewidth = (bpp + 7) / 8, linelength = 1 + ((bpp * passw + 7) / 8);
			for (unsigned long y = 0; y < passh; y++)
			{
				unsigned char filterType = in[y * linelength], * prevline = (y == 0) ? 0 : lineo;
				unFilterScanline(linen, &in[y * linelength + 1], prevline, bytewidth, filterType, (w * bpp + 7) / 8); if (error) return;
				if (bpp >= 8) for (size_t i = 0; i < passw; i++) for (size_t b = 0; b < bytewidth; b++) //b = current byte of this pixel
					out[bytewidth * w * (passtop + spacey * y) + bytewidth * (passleft + spacex * i) + b] = linen[bytewidth * i + b];
				else for (size_t i = 0; i < passw; i++)
				{
					size_t obp = bpp * w * (passtop + spacey * y) + bpp * (passleft + spacex * i), bp = i * bpp;
					for (size_t b = 0; b < bpp; b++) setBitOfReversedStream(obp, out, readBitFromReversedStream(bp, &linen[0]));
				}
				unsigned char* temp = linen; linen = lineo; lineo = temp; //swap the two buffer pointers "line old" and "line new"
			}
		}
		static unsigned long readBitFromReversedStream(size_t& bitp, const unsigned char* bits) { unsigned long result = (bits[bitp >> 3] >> (7 - (bitp & 0x7))) & 1; bitp++; return result; }
		static unsigned long readBitsFromReversedStream(size_t& bitp, const unsigned char* bits, unsigned long nbits)
		{
			unsigned long result = 0;
			for (size_t i = nbits - 1; i < nbits; i--) result += ((readBitFromReversedStream(bitp, bits)) << i);
			return result;
		}
		void setBitOfReversedStream(size_t& bitp, unsigned char* bits, unsigned long bit) { bits[bitp >> 3] |= (bit << (7 - (bitp & 0x7))); bitp++; }
		unsigned long read32bitInt(const unsigned char* buffer) { return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]; }
		int checkColorValidity(unsigned long colorType, unsigned long bd) //return type is a LodePNG error code
		{
			if ((colorType == 2 || colorType == 4 || colorType == 6)) { if (!(bd == 8 || bd == 16)) return 37; else return 0; }
			else if (colorType == 0) { if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; else return 0; }
			else if (colorType == 3) { if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8)) return 37; else return 0; }
			else return 31; //unexisting color type
		}
		unsigned long getBpp(const Info& info)
		{
			if (info.colorType == 2) return (3 * info.bitDepth);
			else if (info.colorType >= 4) return (info.colorType - 2) * info.bitDepth;
			else return info.bitDepth;
		}
		int convert(std::vector<unsigned char>& out, const unsigned char* in, Info& infoIn, unsigned long w, unsigned long h)
		{ //converts from any color type to 32-bit. return value = LodePNG error code
			size_t numpixels = w * h, bp = 0;
			out.resize(numpixels * 4);
			unsigned char* out_ = out.empty() ? 0 : &out[0]; //faster if compiled without optimization
			if (infoIn.bitDepth == 8 && infoIn.colorType == 0) //greyscale
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[i];
					out_[4 * i + 3] = (infoIn.key_defined && in[i] == infoIn.key_r) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 2) //RGB color
				for (size_t i = 0; i < numpixels; i++)
				{
					for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[3 * i + c];
					out_[4 * i + 3] = (infoIn.key_defined == 1 && in[3 * i + 0] == infoIn.key_r && in[3 * i + 1] == infoIn.key_g && in[3 * i + 2] == infoIn.key_b) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 3) //indexed color (palette)
				for (size_t i = 0; i < numpixels; i++)
				{
					if (4U * in[i] >= infoIn.palette.size()) return 46;
					for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * in[i] + c]; //get rgb colors from the palette
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 4) //greyscale with alpha
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i + 0];
					out_[4 * i + 3] = in[2 * i + 1];
				}
			else if (infoIn.bitDepth == 8 && infoIn.colorType == 6) for (size_t i = 0; i < numpixels; i++) for (size_t c = 0; c < 4; c++) out_[4 * i + c] = in[4 * i + c]; //RGB with alpha
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 0) //greyscale
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[2 * i];
					out_[4 * i + 3] = (infoIn.key_defined && 256U * in[i] + in[i + 1] == infoIn.key_r) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 2) //RGB color
				for (size_t i = 0; i < numpixels; i++)
				{
					for (size_t c = 0; c < 3; c++) out_[4 * i + c] = in[6 * i + 2 * c];
					out_[4 * i + 3] = (infoIn.key_defined && 256U * in[6 * i + 0] + in[6 * i + 1] == infoIn.key_r && 256U * in[6 * i + 2] + in[6 * i + 3] == infoIn.key_g && 256U * in[6 * i + 4] + in[6 * i + 5] == infoIn.key_b) ? 0 : 255;
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 4) //greyscale with alpha
				for (size_t i = 0; i < numpixels; i++)
				{
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = in[4 * i]; //most significant byte
					out_[4 * i + 3] = in[4 * i + 2];
				}
			else if (infoIn.bitDepth == 16 && infoIn.colorType == 6) for (size_t i = 0; i < numpixels; i++) for (size_t c = 0; c < 4; c++) out_[4 * i + c] = in[8 * i + 2 * c]; //RGB with alpha
			else if (infoIn.bitDepth < 8 && infoIn.colorType == 0) //greyscale
				for (size_t i = 0; i < numpixels; i++)
				{
					unsigned long value = (readBitsFromReversedStream(bp, in, infoIn.bitDepth) * 255) / ((1 << infoIn.bitDepth) - 1); //scale value from 0 to 255
					out_[4 * i + 0] = out_[4 * i + 1] = out_[4 * i + 2] = (unsigned char)(value);
					out_[4 * i + 3] = (infoIn.key_defined && value && ((1U << infoIn.bitDepth) - 1U) == infoIn.key_r && ((1U << infoIn.bitDepth) - 1U)) ? 0 : 255;
				}
			else if (infoIn.bitDepth < 8 && infoIn.colorType == 3) //palette
				for (size_t i = 0; i < numpixels; i++)
				{
					unsigned long value = readBitsFromReversedStream(bp, in, infoIn.bitDepth);
					if (4 * value >= infoIn.palette.size()) return 47;
					for (size_t c = 0; c < 4; c++) out_[4 * i + c] = infoIn.palette[4 * value + c]; //get rgb colors from the palette
				}
			return 0;
		}
		unsigned char paethPredictor(short a, short b, short c) //Paeth predicter, used by PNG filter type 4
		{
			short p = a + b - c, pa = p > a ? (p - a) : (a - p), pb = p > b ? (p - b) : (b - p), pc = p > c ? (p - c) : (c - p);
			return (unsigned char)((pa <= pb && pa <= pc) ? a : pb <= pc ? b : c);
		}
	};
	PNG decoder; decoder.decode(out_image, in_png, in_size, convert_to_rgba32);
	image_width = decoder.info.width; image_height = decoder.info.height;
	return decoder.error;
}

int decodePNG(std::vector<unsigned char>& out_image_32bit, unsigned long& image_width, unsigned long& image_height, const std::vector<unsigned char>& in_png)
{
	return decodePNG(out_image_32bit, image_width, image_height, static_cast<const unsigned char*>(&in_png[0]), int(in_png.size()));
}