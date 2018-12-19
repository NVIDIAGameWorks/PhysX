uniform mat4 g_modelViewMatrix;
uniform mat4 g_projMatrix;

attribute vec3 position_attr;
attribute vec4 color_attr;
attribute vec2 texcoord0_attr;

varying vec2 texcoord0;
varying vec4 color;

void main()
{
    gl_Position = vec4(position_attr, 1.0);
    texcoord0 = texcoord0_attr;
	color = color_attr;
}