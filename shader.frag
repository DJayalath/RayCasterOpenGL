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

#define NUM_TEXTURES 11
#define TEX_WIDTH 64
#define TEX_HEIGHT 64

uniform int worldmap[MAPWIDTH * MAPHEIGHT];

 layout(std430, binding = 3) buffer dataLayout
 {
     uint data_SSBO[];
 };

void main()
{
	uint color = data_SSBO[int(gl_FragCoord.y) * W_WIDTH + int(gl_FragCoord.x)];
	pxColour = vec4(float(color & 0xFF) / 255.f, float((color & 0xFF00) >> 8) / 255.f, float((color & 0xFF0000) >> 16) / 255.f, 1.f);
}