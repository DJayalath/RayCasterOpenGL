# RayCasterOpenGL
3D ray casting engine written using GLSL shader code.

All ray casting calculations done on GPU fragment shader (see ``shader.frag``)

See [NoOpenGL](https://github.com/armytricks/RayCasterOpenGL/tree/NoOpenGL) branch for a version that does not use OpenGL and supports sprites.

See [CPU_Casting](https://github.com/armytricks/RayCasterOpenGL/tree/CPU_Casting) branch for a version that does ray casting on the CPU and renders using OpenGL. This is faster than ``NoOpenGL`` and can support sprites in a future implementation.

Based on https://lodev.org/cgtutor/raycasting.html

## Textured
![Textured screenshot](https://raw.githubusercontent.com/armytricks/3DRayCastingEngine/master/scr_textured.png)

## Untextured
![Untextured screenshot](https://raw.githubusercontent.com/armytricks/3DRayCastingEngine/master/scr_untextured.png)
