uniform mat4 g_modelViewMatrix;
uniform mat4 g_projMatrix;

attribute vec3 position_attr;
attribute vec3 normal_attr;
attribute vec2 texcoord0_attr;

varying vec2 texcoord0;
varying vec4 lighting;
varying vec4 normal;

void main()
{
    gl_Position = g_projMatrix * g_modelViewMatrix * vec4(position_attr, 1.0);
	normal = normalize(g_modelViewMatrix * vec4(normal_attr, 0.0));
	float lighting_scalar = clamp(normal.z, 0.4, 1.0);
    lighting = vec4(lighting_scalar, lighting_scalar, lighting_scalar, 1.0);
    texcoord0 = texcoord0_attr;	
}