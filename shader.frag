#version 430 core

in vec4 gl_FragCoord;
out vec4 pxColour;

#define W_WIDTH 1280
#define W_HEIGHT 720

 layout(std430, binding = 3) buffer dataLayout
 {
     uint data_SSBO[];
 };

void main()
{
	uint color = data_SSBO[int(gl_FragCoord.y) * W_WIDTH + int(gl_FragCoord.x)];
	pxColour = vec4(float(color & 0xFF) / 255.f, float((color & 0xFF00) >> 8) / 255.f, float((color & 0xFF0000) >> 16) / 255.f, 1.f);
}