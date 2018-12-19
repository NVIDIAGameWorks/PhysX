uniform mat4 g_modelMatrix;

attribute vec3 position_attr;
attribute vec2 texcoord0_attr;

varying vec2 texcoord0;

void main()
{
    gl_Position = vec4(position_attr, 1.0) + g_modelMatrix[0];
    texcoord0 = texcoord0_attr;
}