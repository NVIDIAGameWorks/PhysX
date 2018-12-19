#define MAX_BONES 50
uniform mat4 g_boneMatrices[MAX_BONES];

uniform mat4 g_modelViewMatrix;
uniform mat4 g_projMatrix;
attribute float boneIndex_attr;
attribute vec3 position_attr;
attribute vec3 normal_attr;
attribute vec2 texcoord0_attr;

varying vec4 color;
varying vec2 texcoord0;


void main()
{
    int bi = int(boneIndex_attr);
    gl_Position = g_projMatrix * g_modelViewMatrix * g_boneMatrices[bi] * vec4(position_attr, 1.0);
    vec4 normal2 = normalize(g_modelViewMatrix * g_boneMatrices[bi] * vec4(normal_attr, 0.0));
    color = clamp(normal2.z, 0.2, 1.0) * vec4(1.0, 1.0, 1.0, 1.0);

    texcoord0 = texcoord0_attr;
}