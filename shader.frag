#version 430 core

in vec4 gl_FragCoord;
out vec4 pxColour;

uniform vec2 dir;
uniform vec2 plane;
uniform vec2 pos;

#define MAPWIDTH 24
#define MAPHEIGHT 24

#define W_WIDTH 1280
#define W_HEIGHT 720

#define NUM_TEXTURES 8
#define TEX_WIDTH 64
#define TEX_HEIGHT 64

uniform int worldmap[MAPWIDTH * MAPHEIGHT];

 layout(std430, binding = 3) buffer dataLayout
 {
     uint data_SSBO[];
 };

void main()
{
	int x = int(gl_FragCoord.x);
	int y = int(gl_FragCoord.y);

	float cameraX = 2.f * x / W_WIDTH - 1.f;
	vec2 rayDir = dir + plane * cameraX;

	// DDA vars
	ivec2 map = ivec2(pos);
	vec2 sideDist;
	vec2 deltaDist = abs(1.0f / rayDir);
	float perpWallDist;
	ivec2 ddaStep;
	bool hit = false;
	int side = 0;

	// DDA setup
	if (rayDir.x < 0)
	{
		ddaStep.x = -1;
		sideDist.x = (pos.x - map.x) * deltaDist.x;
	}
	else
	{
		ddaStep.x = 1;
		sideDist.x = (map.x + 1.f - pos.x) * deltaDist.x;
	}
	if (rayDir.y < 0)
	{
		ddaStep.y = -1;
		sideDist.y = (pos.y - map.y) * deltaDist.y;
	}
	else
	{
		ddaStep.y = 1;
		sideDist.y = (map.y + 1.f - pos.y) * deltaDist.y;
	}

	// DDA Execution
	while (!hit)
	{
		// jump to next map square, OR in x-direction, OR in y-direction
		if (sideDist.x < sideDist.y)
		{
			sideDist.x += deltaDist.x;
			map.x += ddaStep.x;
			side = 0;
		}
		else
		{
			sideDist.y += deltaDist.y;
			map.y += ddaStep.y;
			side = 1;
		}

		//Check if ray has hit a wall
		if (worldmap[map.y * MAPWIDTH + map.x] > 0) hit = true;
	}

	//Calculate distance projected on camera direction (Euclidean distance will give fisheye effect!)
	if (side == 0)
	{
		perpWallDist = (map.x - pos.x + (1.f - ddaStep.x) / 2.f) / rayDir.x;
	}
	else
	{
		perpWallDist = (map.y - pos.y + (1.f - ddaStep.y) / 2.f) / rayDir.y;
	}

	// Limit to prevent overflow
	if (perpWallDist < 0.0001f)
		perpWallDist = 0.0001f;

	// Calculate height of line to draw on screen
	// Also determines wall height (h * a) where a is height multiplier
	int lineHeight = int(W_HEIGHT / perpWallDist);

	// Limit lineHeight to prevent integer overflow
	if (lineHeight > 0xFFFF)
		lineHeight = 0xFFFF;

	//calculate lowest and highest pixel to fill in current stripe
	int drawStart = -lineHeight / 2 + W_HEIGHT / 2;
	if (drawStart < 0) drawStart = 0;
	int drawEnd = lineHeight / 2 + W_HEIGHT / 2;
	if (drawEnd >= W_HEIGHT) drawEnd = W_HEIGHT - 1;

	// TEXTURING
	//texturing calculations
	int texNum = worldmap[map.x + map.y * MAPWIDTH] - 1; //1 subtracted from it so that texture 0 can be used!

	//calculate value of wallX
	float wallX; //where exactly the wall was hit
	if (side == 0) wallX = pos.y + perpWallDist * rayDir.y;
	else           wallX = pos.x + perpWallDist * rayDir.x;
	wallX -= floor(wallX);

	//x coordinate on the texture
	int texX = int(wallX * float(TEX_WIDTH));
	if (side == 0 && rayDir.x > 0) texX = TEX_WIDTH - texX - 1;
	if (side == 1 && rayDir.y < 0) texX = TEX_WIDTH - texX - 1;

	float inv_lineHeight = 1.f / lineHeight;

	int d = y * 256 - W_HEIGHT * 128 + lineHeight * 128;  //256 and 128 factors to avoid floats
	// TODO: avoid the division to speed this up
	int texY = int(((d * TEX_HEIGHT) * inv_lineHeight)) >> 8;

	uint color = data_SSBO[texNum * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * texY + texX];
	
	if (y >= drawStart && y <= drawEnd)
	{
		// make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
		if (side == 1)
		{
			color = (color >> 1) & 8355711;
		}

		pxColour = vec4(float(color & 0xFF) / 255.f, float((color & 0xFF00) >> 8) / 255.f, float((color & 0xFF0000) >> 16) / 255.f, 1.f);
	}
	else
	{
		// FLOOR AND CEILING
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

		if (y < drawStart)
		{
			currentDist = -W_HEIGHT / (2.0 * y - W_HEIGHT); //you could make a small lookup table for this instead

			double weight = (currentDist - distPlayer) / (distWall - distPlayer);

			double currentFloorX = weight * floorXWall + (1.0 - weight) * pos.x;
			double currentFloorY = weight * floorYWall + (1.0 - weight) * pos.y;

			// Division by 4 to scale texture for speed;
			int floorTexX, floorTexY;
			floorTexX = int(currentFloorX * TEX_WIDTH / 4) % TEX_WIDTH;
			floorTexY = int(currentFloorY * TEX_HEIGHT / 4) % TEX_HEIGHT;

			uint color = data_SSBO[3 * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * floorTexY + floorTexX];
			pxColour = vec4(float(color & 0xFF) / 255.f, float((color & 0xFF00) >> 8) / 255.f, float((color & 0xFF0000) >> 16) / 255.f, 1.f);

		}
		else if (y > drawEnd)
		{
			currentDist = W_HEIGHT / (2.0 * y - W_HEIGHT); //you could make a small lookup table for this instead

			double weight = (currentDist - distPlayer) / (distWall - distPlayer);

			double currentFloorX = weight * floorXWall + (1.0 - weight) * pos.x;
			double currentFloorY = weight * floorYWall + (1.0 - weight) * pos.y;

			// Division by 4 to scale texture for speed;
			int floorTexX, floorTexY;
			floorTexX = int(currentFloorX * TEX_WIDTH / 4) % TEX_WIDTH;
			floorTexY = int(currentFloorY * TEX_HEIGHT / 4) % TEX_HEIGHT;

			uint color = data_SSBO[6 * TEX_WIDTH * TEX_HEIGHT + TEX_HEIGHT * floorTexY + floorTexX];
			pxColour = vec4(float(color & 0xFF) / 255.f, float((color & 0xFF00) >> 8) / 255.f, float((color & 0xFF0000) >> 16) / 255.f, 1.f);

		}

	}
}