//const vec2 offsets[4] = vec2[4] { vec2(1, 1), vec2(-1, 1), vec2(-1, -1), vec2(1, -1) };
//const vec2 tc[4] = vec2[4] { vec2(1, 1), vec2(0, 1),  vec2(0, 0),   vec2(1, 0) };

uniform float windowWidth;
uniform float particleSizeScale;

uniform mat4 g_viewMatrix;
uniform mat4 g_modelMatrix;
uniform mat4 g_projMatrix;

attribute vec4 color_attr;
attribute float psize;
attribute vec3 position_attr;
attribute vec2 texcoord0_attr;
attribute vec2 texcoord1_attr;
attribute vec2 texcoord2_attr;
attribute vec2 texcoord3_attr;

varying vec4 color;
varying vec2 texcoord0;
varying vec2 texcoord1;
varying vec2 texcoord2;
varying vec2 texcoord3;

/*
float clampedGraph(float value, float vmin, float vmax)
{
	return saturate((value - vmin) / (vmax-vmin));
}
*/
void main()
{
	
	mat4 modelMatrix;
#if RENDERER_INSTANCED
	modelMatrix = transpose(vec4x4(vec4(instanceNormalX, 0), vec4(instanceNormalY, 0), vec4(instanceNormalZ, 0), vec4(instanceOffset, 1)));
#else
	modelMatrix = g_modelMatrix;
#endif
	
	int index                        = int(texcoord2_attr.x);
	vec2 offset;
    switch (index)
    {
        case 0: offset = vec2(1, 1); break;
        case 1: offset = vec2(-1, 1); break;
        case 2: offset = vec2(-1, -1); break;
        case 3: offset = vec2(1, -1); break;
    }

	vec4 viewPos                     = g_viewMatrix * vec4(position_attr, 1.0);
    viewPos.xy                       = viewPos.xy - offset.xy * texcoord2_attr.y;
	vec4 screenSpacePosition         = g_projMatrix * viewPos;
    vec3 worldSpacePosition          = (g_modelMatrix * vec4(position_attr, 1.0)).xyz;

    gl_Position = screenSpacePosition;
/*
	float3 tbz = normalize(g_eyePosition - vout.params.worldSpacePosition);
	float3 tbx = cross(float3(0,1,0), tbz);
	
	vout.params.worldSpaceNormal   = tbz;//normalize(mul(modelMatrix, vec4(localSpaceNormal,    0)).xyz);
	vout.params.worldSpaceTangent  = tbx;//normalize(mul(g_modelMatrix, vec4(localSpaceTangent, 0)).xyz);
	vout.params.worldSpaceBinormal = cross(vout.params.worldSpaceNormal, vout.params.worldSpaceTangent);

	float3x3 tangentBasis = float3x3(vout.params.worldSpaceTangent, vout.params.worldSpaceBinormal, vout.params.worldSpaceNormal);
	vout.params.worldSpaceBinormal = mul(tangentBasis, g_lightDirection);
*/	
	texcoord0   = (offset + vec2(1,1)) * 0.5;//tc[index];
	texcoord1   = texcoord1_attr;
	texcoord2   = texcoord2_attr;
	texcoord3   = texcoord3_attr;
//	vout.params.color       = vertexColor;
    color = color_attr;
	
	//gl_PointSize          = (g_projMatrix[0][0] / screenSpacePosition.w) * (windowWidth / 2.0) * psize * particleSizeScale;
}
