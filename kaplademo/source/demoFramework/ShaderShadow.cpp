//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

#include "ShaderShadow.h"
//#define NOMINMAX
#include <windows.h>
#include "ShadowMap.h"
#include "foundation/PxMat44.h"
#include "Compound.h"

ShaderShadow::RenderMode ShaderShadow::renderMode = ShaderShadow::RENDER_COLOR;
float ShaderShadow::hdrScale = 1.f;
GLuint ShaderShadow::skyBoxTex = 0;
//float g_shadowAdd = -0.00001f;
//float g_shadowAdd = -0.003349f;
float g_shadowAdd = -0.00001f;
// --------------------------------------------------------------------------------------------
const char *crossHairVertexProgram = STRINGIFY(
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
);
const char *crossHairFragmentProgram = STRINGIFY(
uniform sampler2D texture;
void main()
{
	vec4 color = texture2D(texture, gl_TexCoord[0]);
	gl_FragColor = vec4(1.0,1.0,1.0, 1.0f-color.x);
	
}
);

// --------------------------------------------------------------------------------------------
const char *shadowDiffuseVertexProgramTexInstance = STRINGIFY(

uniform float uvScale = 1.0f;
uniform sampler2D transTex;
uniform int transTexSize;
uniform float iTransTexSize;
uniform float bumpTextureUVScale;
//attribute mat4 transformmatrix;
void main()
{
	
	int ti = (int)(gl_MultiTexCoord0.w);
	//int ti = tq;
	int tpr = transTexSize / 4;
	int row = ti / tpr;
	int col = (ti - row*tpr)*4;

	float fx = (col+0.5f)*iTransTexSize;
	float fy = (row+0.5f)*iTransTexSize;

	
	vec4 r0 = texture2D(transTex, vec2(fx,fy));
	vec4 r1 = texture2D(transTex, vec2(fx+iTransTexSize,fy));
	vec4 r2 = texture2D(transTex, vec2(fx+iTransTexSize*2.0f,fy));
	vec4 r3 = texture2D(transTex, vec2(fx+iTransTexSize*3.0f,fy));
//	vec4 r3 = vec4(0,0,0,1);
	
	vec3 offset = vec3(r0.w, r1.w, r2.w);
	r0.w = 0.0f;
	r1.w = 0.0f;
	r2.w = 0.0f;

	float material = r3.w;
	r3.w = 1.0f;
	mat4 transformmatrix = mat4(r0,r1,r2,r3);


	

	mat4 mvp = gl_ModelViewMatrix * transformmatrix;
	mat4 mvpt = gl_ModelViewMatrixInverseTranspose * transformmatrix;
	vec4 t0 = vec4(gl_MultiTexCoord0.xyz, 0.0f);

	vec4 t1 = vec4(cross(gl_Normal.xyz, t0.xyz), 0.0f);

//	mat4 mvp = gl_ModelViewMatrix;
//	mat4 mvpt = gl_ModelViewMatrixInverseTranspose;


	vec4 eyeSpacePos = mvp * gl_Vertex;
	//eyeSpacePos.y += gl_InstanceID * 0.2f;
	//gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz*uvScale;
	vec3 coord3d = gl_Vertex.xyz + offset;
	gl_TexCoord[0].xyz = (coord3d)*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 

	gl_TexCoord[3] = mvpt * t0;
	gl_TexCoord[4].xyz = mvpt * t1;

	gl_TexCoord[5].xy = vec2(dot(coord3d,  t0.xyz), dot(coord3d,  t1.xyz))*bumpTextureUVScale*2;
	
	gl_TexCoord[6].xyz = vec3(gl_MultiTexCoord1.xy, material);
	gl_TexCoord[6].y = 1.0 - gl_TexCoord[6].y;

	float MAX_3D_TEX = 8.0;
	if (gl_TexCoord[6].x >= 5.0f) {
		// 2D Tex
		gl_TexCoord[6].x -= 5.0f;		
		gl_TexCoord[6].z = floor(gl_TexCoord[6].z / MAX_3D_TEX);
	} else {
		gl_TexCoord[6].z -= floor(gl_TexCoord[6].z / MAX_3D_TEX)*MAX_3D_TEX;
		gl_TexCoord[6].z -= 100.0f;
		
	}
	gl_TexCoord[6].w = floor(fract(material / MAX_3D_TEX)*MAX_3D_TEX + 0.5f);
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
);

// --------------------------------------------------------------------------------------------
const char *shadowDiffuseVertexProgramInstance = STRINGIFY(

uniform float uvScale = 1.0f;
attribute mat4 transformmatrix;
uniform float bumpTextureUVScale;

void main()
{
	mat4 mvp = gl_ModelViewMatrix * transformmatrix;
	mat4 mvpt = gl_ModelViewMatrixInverseTranspose * transformmatrix;
	//mat4 mvp2 = gl_ModelViewMatrix * transformmatrix;
	//mat4 mvp = gl_ModelViewMatrix;
	//mat4 mvpt = gl_ModelViewMatrixInverseTranspose;	

	vec4 eyeSpacePos = mvp * gl_Vertex;

	vec4 t0 = vec4(gl_MultiTexCoord0.xyz, 0.0f);
	vec4 t1 = vec4(cross(gl_Normal.xyz, t0.xyz), 0.0f);


	vec3 coord3d = gl_Vertex.xyz;
	gl_TexCoord[0].xyz = (coord3d)*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 

	gl_TexCoord[3] = mvpt * t0;
	gl_TexCoord[4] = mvpt * t1;

	gl_TexCoord[5].xy = vec2(dot(coord3d,  t0.xyz), dot(coord3d,  t1.xyz))*bumpTextureUVScale*2;

	gl_TexCoord[6].xyz = vec3(0,0,-100); // TODO: 2D UV are 0 and material id is -100 (first 3D texture)

	/*
	//vec4 eyeSpacePos2 = mvp2 * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	//gl_FrontColor.x += eyeSpacePos2.x;
	
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
	*/
}
);


// --------------------------------------------------------------------------------------------
const char *shadowDiffuseVertexProgramFor3DTex = STRINGIFY(
	uniform float uvScale = 1.0f;
	uniform float bumpTextureUVScale;
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	
	gl_TexCoord[1] = eyeSpacePos;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
	
	vec3 coord3d = gl_Vertex.xyz;
	
	vec4 t0 = vec4(normalize(cross(vec3(1,0,0),gl_Normal.xyz)) ,0.0f);
	vec4 t1 = vec4(normalize(cross(t0.xyz,gl_Normal.xyz)), 0.0f);

	gl_TexCoord[0].xyz = coord3d*uvScale;
	gl_TexCoord[3] = gl_ModelViewMatrixInverseTranspose * t0;
	gl_TexCoord[4] = gl_ModelViewMatrixInverseTranspose * t1;
	gl_FrontColor = gl_Color;

	gl_TexCoord[5].xy = vec2(dot(coord3d,  t0.xyz), dot(coord3d,  t1.xyz))*bumpTextureUVScale*2;
	gl_TexCoord[6].xyz = vec3(0,0,-100); 

}
);

// --------------------------------------------------------------------------------------------
const char *shadowDiffuseVertexProgram = STRINGIFY(
uniform float uvScale = 1.0f;
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_TexCoord[3].xyz = gl_Vertex.xyz;
	gl_TexCoord[4].xyz = gl_Normal.xyz;
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
);

// --------------------------------------------------------------------------------------------
const char *shadowDiffuseVertexProgram4Bones = STRINGIFY(
uniform float uvScale = 1.0f;
uniform sampler2D transTex;
uniform float transvOffset;
uniform float texelSpacing;

void getPosNormal(vec2 uv, vec4 vertex, vec4 normal, 
				  out vec4 posOut, out vec4 normalOut) {
	  vec4 f0;
	  vec4 f1;
	  vec4 f2;
	  f0 = texture2D(transTex, uv);
	  f1 = texture2D(transTex, vec2(uv.x + texelSpacing, uv.y));
	  f2 = texture2D(transTex, vec2(uv.x + 2.0f*texelSpacing, uv.y));
	  
	  posOut = vec4(dot(f0, vertex), dot(f1, vertex), dot(f2, vertex), 1.0f);

	  normalOut = vec4(dot(f0, normal), dot(f1, normal),  dot(f2, normal), 0.0f);
	  
}
void main()
{

	vec2 t0uv = vec2(gl_MultiTexCoord1.x, gl_MultiTexCoord1.y + transvOffset);
	vec2 t1uv = vec2(gl_MultiTexCoord1.z, gl_MultiTexCoord1.w + transvOffset);
	vec2 t2uv = vec2(gl_MultiTexCoord2.x, gl_MultiTexCoord2.y + transvOffset);
	vec2 t3uv = vec2(gl_MultiTexCoord2.z, gl_MultiTexCoord2.w + transvOffset);
	float t0w = gl_MultiTexCoord3.x;
	float t1w = gl_MultiTexCoord3.y;
	float t2w = gl_MultiTexCoord3.z;
	float t3w = gl_MultiTexCoord3.w;

	vec4 vert;
	vec4 norm;
	vec4 vert0;
	vec4 norm0;
	vec4 vert1;
	vec4 norm1;
	vec4 vert2;
	vec4 norm2;
	vec4 vert3;
	vec4 norm3;

	getPosNormal(t0uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert0, norm0);
	getPosNormal(t1uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert1, norm1);
	getPosNormal(t2uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert2, norm2);
	getPosNormal(t3uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert3, norm3);

	float sum = max(t0w+t1w+t2w+t3w, 0.001f);
	vert = (t0w*vert0 + t1w*vert1 + t2w*vert2 + t3w*vert3)/sum;
	norm = t0w*norm0 + t1w*norm1 + t2w*norm2 + t3w*norm3;
	norm = vec4(normalize(norm.xyz), 0.0f);
	//vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	vec4 eyeSpacePos = gl_ModelViewMatrix * vert;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	//gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * norm; 
	gl_TexCoord[3] = gl_MultiTexCoord4;
	gl_TexCoord[4] = gl_MultiTexCoord5;
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
);

// --------------------------------------------------------------------------------------------

const char *shadowDiffuseVertexProgram8BonesTangent = STRINGIFY(
	uniform float uvScale = 1.0f;
uniform sampler2D transTex;
uniform float transvOffset;
uniform float texelSpacing;
/*
void getPosNormalTangent(vec2 uv, vec4 vertex, vec4 normal, vec4 tangent,
				  out vec4 posOut, out vec4 normalOut, out vec4 tangentOut) {
					  vec4 f0;
					  vec4 f1;
					  vec4 f2;
					  f0 = texture2D(transTex, uv);
					  f1 = texture2D(transTex, vec2(uv.x + texelSpacing, uv.y));
					  f2 = texture2D(transTex, vec2(uv.x + 2.0f*texelSpacing, uv.y));

					  posOut = vec4(dot(f0, vertex), dot(f1, vertex), dot(f2, vertex), 1.0f);

					  normalOut = vec4(dot(f0, normal), dot(f1, normal),  dot(f2, normal), 0.0f);

					  tangentOut = vec4(dot(f0, tangent), dot(f1, tangent),  dot(f2, tangent), 0.0f);

}
*/
void getPosNormal(vec2 uv, vec4 vertex, vec4 normal, 
				  out vec4 posOut, out vec4 normalOut) {
					  vec4 f0;
					  vec4 f1;
					  vec4 f2;
					  f0 = texture2D(transTex, uv);
					  f1 = texture2D(transTex, vec2(uv.x + texelSpacing, uv.y));
					  f2 = texture2D(transTex, vec2(uv.x + 2.0f*texelSpacing, uv.y));

					  posOut = vec4(dot(f0, vertex), dot(f1, vertex), dot(f2, vertex), 1.0f);

					  normalOut = vec4(dot(f0, normal), dot(f1, normal),  dot(f2, normal), 0.0f);

}
void main()
{

	vec2 t0uv = vec2(gl_MultiTexCoord1.x, gl_MultiTexCoord1.y + transvOffset);
	vec2 t1uv = vec2(gl_MultiTexCoord1.z, gl_MultiTexCoord1.w + transvOffset);
	vec2 t2uv = vec2(gl_MultiTexCoord2.x, gl_MultiTexCoord2.y + transvOffset);
	vec2 t3uv = vec2(gl_MultiTexCoord2.z, gl_MultiTexCoord2.w + transvOffset);
	vec2 t4uv = vec2(gl_MultiTexCoord3.x, gl_MultiTexCoord3.y + transvOffset);
	vec2 t5uv = vec2(gl_MultiTexCoord3.z, gl_MultiTexCoord3.w + transvOffset);
	vec2 t6uv = vec2(gl_MultiTexCoord4.x, gl_MultiTexCoord4.y + transvOffset);
	vec2 t7uv = vec2(gl_MultiTexCoord4.z, gl_MultiTexCoord4.w + transvOffset);

	float t0w = gl_MultiTexCoord5.x;
	float t1w = gl_MultiTexCoord5.y;
	float t2w = gl_MultiTexCoord5.z;
	float t3w = gl_MultiTexCoord5.w;
	float t4w = gl_MultiTexCoord6.x;
	float t5w = gl_MultiTexCoord6.y;
	float t6w = gl_MultiTexCoord6.z;
	float t7w = gl_MultiTexCoord6.w;

	vec4 vert;
	vec4 norm;
	vec4 vert0;
	vec4 norm0;
	vec4 vert1;
	vec4 norm1;
	vec4 vert2;
	vec4 norm2;
	vec4 vert3;
	vec4 norm3;

	vec4 vert4;
	vec4 norm4;
	vec4 vert5;
	vec4 norm5;
	vec4 vert6;
	vec4 norm6;
	vec4 vert7;
	vec4 norm7;

	/*
	vec4 tangent0;
	vec4 tangent1;
	vec4 tangent2;
	vec4 tangent3;
	vec4 tangent4;
	vec4 tangent5;
	vec4 tangent6;
	vec4 tangent7;
	*/
	getPosNormal(t0uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert0, norm0);
	getPosNormal(t1uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert1, norm1);
	getPosNormal(t2uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert2, norm2);
	getPosNormal(t3uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert3, norm3);

	getPosNormal(t4uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert4, norm4);
	getPosNormal(t5uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert5, norm5);
	getPosNormal(t6uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert6, norm6);
	getPosNormal(t7uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert7, norm7);

	/*
	getPosNormalTangent(t0uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert0, norm0, tangent0);
	getPosNormalTangent(t1uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert1, norm1, tangent1);
	getPosNormalTangent(t2uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert2, norm2, tangent2);
	getPosNormalTangent(t3uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert3, norm3, tangent3);

	getPosNormalTangent(t4uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert4, norm4, tangent4);
	getPosNormalTangent(t5uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert5, norm5, tangent5);
	getPosNormalTangent(t6uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert6, norm6, tangent6);
	getPosNormalTangent(t7uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vec4(gl_MultiTexCoord7.xyz, 0.0), vert7, norm7, tangent7);
*/
	float sum = max(t0w+t1w+t2w+t3w+t4w+t5w+t6w+t7w, 0.001f);
	vert = (t0w*vert0 + t1w*vert1 + t2w*vert2 + t3w*vert3 + t4w*vert4 + t5w*vert5 + t6w*vert6 + t7w*vert7)/sum;
	norm = t0w*norm0 + t1w*norm1 + t2w*norm2 + t3w*norm3 + t4w*norm4 + t5w*norm5 + t6w*norm6 + t7w*norm7;
	norm = vec4(normalize(norm.xyz), 0.0);

	/*
	vec4 tangent = t0w*tangent0 + t1w*tangent1 + t2w*tangent2 + t3w*tangent3 + t4w*tangent4 + t5w*tangent5 + t6w*tangent6 + t7w*tangent7;
	tangent = vec4(normalize(tangent.xyz), 0.0);
	
	vec3 bitangent =  normalize(cross(norm.xyz, tangent.xyz));
	tangent.xyz = cross(bitangent, norm.xyz);
	*/

	//vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	vec4 eyeSpacePos = gl_ModelViewMatrix * vert;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	//gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * norm; 
	//gl_TexCoord[3] = gl_ModelViewMatrixInverseTranspose * tangent;
	//gl_TexCoord[4] = gl_ModelViewMatrixInverseTranspose * vec4(bitangent, 0.0);
	//gl_TexCoord[4] = gl_MultiTexCoord8;

	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
);


const char *shadowDiffuseVertexProgram8Bones = STRINGIFY(
	uniform float uvScale = 1.0f;
uniform sampler2D transTex;
uniform float transvOffset;
uniform float texelSpacing;

void getPosNormal(vec2 uv, vec4 vertex, vec4 normal, 
				  out vec4 posOut, out vec4 normalOut) {
					  vec4 f0;
					  vec4 f1;
					  vec4 f2;
					  f0 = texture2D(transTex, uv);
					  f1 = texture2D(transTex, vec2(uv.x + texelSpacing, uv.y));
					  f2 = texture2D(transTex, vec2(uv.x + 2.0f*texelSpacing, uv.y));

					  posOut = vec4(dot(f0, vertex), dot(f1, vertex), dot(f2, vertex), 1.0f);

					  normalOut = vec4(dot(f0, normal), dot(f1, normal),  dot(f2, normal), 0.0f);

}
void main()
{

	vec2 t0uv = vec2(gl_MultiTexCoord1.x, gl_MultiTexCoord1.y + transvOffset);
	vec2 t1uv = vec2(gl_MultiTexCoord1.z, gl_MultiTexCoord1.w + transvOffset);
	vec2 t2uv = vec2(gl_MultiTexCoord2.x, gl_MultiTexCoord2.y + transvOffset);
	vec2 t3uv = vec2(gl_MultiTexCoord2.z, gl_MultiTexCoord2.w + transvOffset);
	vec2 t4uv = vec2(gl_MultiTexCoord3.x, gl_MultiTexCoord3.y + transvOffset);
	vec2 t5uv = vec2(gl_MultiTexCoord3.z, gl_MultiTexCoord3.w + transvOffset);
	vec2 t6uv = vec2(gl_MultiTexCoord4.x, gl_MultiTexCoord4.y + transvOffset);
	vec2 t7uv = vec2(gl_MultiTexCoord4.z, gl_MultiTexCoord4.w + transvOffset);

	float t0w = gl_MultiTexCoord5.x;
	float t1w = gl_MultiTexCoord5.y;
	float t2w = gl_MultiTexCoord5.z;
	float t3w = gl_MultiTexCoord5.w;
	float t4w = gl_MultiTexCoord6.x;
	float t5w = gl_MultiTexCoord6.y;
	float t6w = gl_MultiTexCoord6.z;
	float t7w = gl_MultiTexCoord6.w;

	vec4 vert;
	vec4 norm;
	vec4 vert0;
	vec4 norm0;
	vec4 vert1;
	vec4 norm1;
	vec4 vert2;
	vec4 norm2;
	vec4 vert3;
	vec4 norm3;

	vec4 vert4;
	vec4 norm4;
	vec4 vert5;
	vec4 norm5;
	vec4 vert6;
	vec4 norm6;
	vec4 vert7;
	vec4 norm7;

	getPosNormal(t0uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert0, norm0);
	getPosNormal(t1uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert1, norm1);
	getPosNormal(t2uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert2, norm2);
	getPosNormal(t3uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert3, norm3);

	getPosNormal(t4uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert4, norm4);
	getPosNormal(t5uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert5, norm5);
	getPosNormal(t6uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert6, norm6);
	getPosNormal(t7uv, gl_Vertex, vec4(gl_Normal.xyz,0.0), vert7, norm7);

	float sum = max(t0w+t1w+t2w+t3w+t4w+t5w+t6w+t7w, 0.001f);
	vert = (t0w*vert0 + t1w*vert1 + t2w*vert2 + t3w*vert3 + t4w*vert4 + t5w*vert5 + t6w*vert6 + t7w*vert7)/sum;
	norm = t0w*norm0 + t1w*norm1 + t2w*norm2 + t3w*norm3 + t4w*norm4 + t5w*norm5 + t6w*norm6 + t7w*norm7;
	norm = vec4(normalize(norm.xyz), 0.0f);
	//vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	vec4 eyeSpacePos = gl_ModelViewMatrix * vert;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	//gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * norm; 
	gl_TexCoord[3] = gl_MultiTexCoord7;
	//gl_TexCoord[4] = gl_MultiTexCoord8;

	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
);

// --------------------------------------------------------------------------------------------

const char *shadowDiffuseFragmentProgram = STRINGIFY(

	// scene reflection 
	uniform float reflectionCoeff = 0.0f;
uniform float specularCoeff = 0.0f;

uniform sampler2DRect reflectionTex;

// Shadow map
uniform float shadowAmbient = 0.0;
uniform sampler2D texture;
uniform sampler2DArrayShadow stex;
uniform sampler2DArrayShadow stex2;
uniform sampler2DArrayShadow stex3;
uniform samplerCube skyboxTex;

uniform float hdrScale = 5.0;

uniform vec2 texSize; // x - size, y - 1/size
uniform vec4 far_d;

// Spot lights
uniform vec3 spotLightDir;
uniform vec3 spotLightPos;
uniform float spotLightCosineDecayBegin;
uniform float spotLightCosineDecayEnd;

uniform vec3 spotLightDir2;
uniform vec3 spotLightPos2;
uniform float spotLightCosineDecayBegin2;
uniform float spotLightCosineDecayEnd2;

uniform vec3 spotLightDir3;
uniform vec3 spotLightPos3;
uniform float spotLightCosineDecayBegin3;
uniform float spotLightCosineDecayEnd3;

uniform vec3 parallelLightDir;
uniform float shadowAdd;
uniform int useTexture;
uniform int numShadows;
uniform vec3 ambientColor;
uniform vec2 shadowTaps[12];
float shadowCoeff1()
{
	//const int index = 0;

	int index = 3;

	if (gl_FragCoord.z < far_d.x)
		index = 0;
	else if (gl_FragCoord.z < far_d.y)
		index = 1;
	else if (gl_FragCoord.z < far_d.z)
		index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);


	// Gaussian 3x3 filter
	//	return shadow2DArray(stex, shadow_coord).x;
	/*
	const float X = 1.0f;
	float ret = shadow2DArray(stex, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;*/
	const int numTaps = 4;
	float radius = 0.0005f / pow(2, index);
	float s = 0.0f;
	for (int i = 0; i < numTaps; i++)
	{
		s += shadow2DArray(stex, shadow_coord + vec4(shadowTaps[i] * radius, 0.0f, 0.0f)).r;
	}
	s /= numTaps;
	return s;
}
float shadowCoeff2()
{
	const int index = 1;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);
	//	return shadow2DArray(stex, shadow_coord).x;
	/*
	const float X = 1.0f;
	float ret = shadow2DArray(stex2, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;*/
	const int numTaps = 12;
	float radius = 1.0f;
	float s = 0.0f;
	for (int i = 0; i < numTaps; i++)
	{
		s += shadow2DArray(stex, shadow_coord + vec4(shadowTaps[i] * radius, 0.0f, 0.0f)).r;
	}
	s /= numTaps;
	return s;
}
float shadowCoeff3()
{
	const int index = 2;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);

	//	return shadow2DArray(stex, shadow_coord).x;
	/*
	const float X = 1.0f;
	float ret = shadow2DArray(stex3, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;*/
	const int numTaps = 12;
	float radius = 0.02f;
	float s = 0.0f;
	for (int i = 0; i < numTaps; i++)
	{
		s += shadow2DArray(stex, shadow_coord + vec4(shadowTaps[i] * radius, 0.0f, 0.0f)).r;
	}
	s /= numTaps;
	return s;
}

float filterwidth(float2 v)
{
	float2 fw = max(abs(ddx(v)), abs(ddy(v)));
	return max(fw.x, fw.y);
}

float2 bump(float2 x)
{
	return (floor((x) / 2) + 2.f * max(((x) / 2) - floor((x) / 2) - .5f, 0.f));
}

float checker(float2 uv)
{
	float width = filterwidth(uv);
	float2 p0 = uv - 0.5 * width;
	float2 p1 = uv + 0.5 * width;

	float2 i = (bump(p1) - bump(p0)) / width;
	return i.x * i.y + (1 - i.x) * (1 - i.y);
}
uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 3.0;   // 5.0 is physically correct

uniform float RollOff = 0.5f;
void main()
{

	//// TODO, expose this as user parameter
	const float skyLightIntensity = 0.2;
	const float rimLightIntensity = 0.3;


	vec3 diffuseMat;
	vec3 specularMat;
	vec3 emissiveReflectSpecPow;

	specularMat = vec3(1.0);
	emissiveReflectSpecPow = vec3(0.0, 0.0, 0.0);

	vec3 normal = normalize(gl_TexCoord[2].xyz);
	vec3 wnormal = normalize(gl_TexCoord[4].xyz);
	// read in material color for diffuse, specular, bump, emmisive

	// 3D texture	
	vec4 colorx;
	if (useTexture > 0)
		colorx = texture2D(texture, gl_TexCoord[0]);
	else {
		colorx = gl_Color;
		if ((wnormal.y >0.9))
		{
			colorx *= 1.0 - 0.25*checker(float2(gl_TexCoord[3].x, gl_TexCoord[3].z));
		}
		else if (abs(wnormal.z) > 0.9)
		{
			colorx *= 1.0 - 0.25*checker(float2(gl_TexCoord[3].y, gl_TexCoord[3].x));
		}
	}
	diffuseMat = colorx.xyz*0.4;
	//diffuseMat = myTexture3D(gl_TexCoord[0].xyz);//texture3D(ttt3D, gl_TexCoord[0].xyz);
	//diffuseMat = texture3D(ttt3D, gl_TexCoord[0].xyz);

	if (dot(normal, gl_TexCoord[1].xyz) > 0) {
		normal.xyz *= -1;
	}

	//gl_FragColor.xyz = normal*0.5 + vec3(0.5,0.5,0.5);
	//gl_FragColor.w = 1;
	//return;
	vec3 eyeVec = normalize(gl_TexCoord[1].xyz);

	// apply gamma correction for diffuse textures
	//diffuseMat = pow(diffuseMat, 0.45);

	float specularPower = emissiveReflectSpecPow.b*255.0f + 1.0f;

	// TODO - fix this
	specularPower = 10.0f;

	float emissive = 0.0f;
	float reflectivity = emissiveReflectSpecPow.b;
	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(normal, eyeVec)), fresnelPower);
	float specular = 0.0f;

	vec3 skyNormal = reflect(eyeVec, normal);
	vec3 skyColor = skyLightIntensity * textureCube(skyboxTex, skyNormal).rgb;
	vec3 ambientSkyColor = diffuseMat * skyColor;

	vec3 diffuseColor = vec3(0.0, 0.0, 0.0);

	if (numShadows >= 1) {

		vec3 lightColor = hdrScale * vec3(1.0, 1.0, 1.0);
		vec3 shadowColor = vec3(0.4, 0.4, 0.7); // colored shadow
		//vec3 lvec = normalize(spotLightDir);
		vec3 lvec = normalize(spotLightPos - gl_TexCoord[1].xyz);
		float cosine = dot(lvec, spotLightDir);
		float intensity = smoothstep(spotLightCosineDecayBegin, spotLightCosineDecayEnd, cosine);

		float ldn = max(0.0f, dot(normal, lvec));
		float shadowC = shadowCoeff1();
		//gl_FragColor = vec4(shadowC,shadowC,shadowC,1.0f);
		//return;
		vec3 irradiance = shadowC * ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance*intensity;

		// add colored shadow
		diffuseColor += (1.0 - shadowC) * shadowAmbient * shadowColor * diffuseMat*intensity;

		vec3 r = reflect(lvec, normal);
		specular += pow(max(0.0, dot(r, eyeVec)), specularPower)*shadowC*intensity;
	}

	// add rim light
	if (numShadows >= 2) {
		vec3 lightColor = hdrScale * vec3(1.0, 1.0, 1.0);
		vec3 lvec = normalize(spotLightDir2);
		float ldn = max(0.0f, dot(normal, lvec));
		vec3 irradiance = ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance;
	}

	vec3 color = vec3(0.0, 0.0, 0.0);

	color += diffuseColor;
	color += ambientSkyColor;
	color += specular*specularMat;
	color += hdrScale * emissive * diffuseMat;

	//vec3 reflectColor = diffuseMat * texture2DRect(reflectionTex, gl_FragCoord.xy).rgb;
	//color = reflectionCoeff * reflectColor + (1.0f - reflectionCoeff) * color;
	color = (fresnel * skyColor + (1.0 - fresnel) * color) * reflectivity + (1.0 - reflectivity) * color;

	gl_FragColor.rgb = color;
	gl_FragColor.w = gl_Color.w;

	//float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	//vec4 fogCol = gl_Fog.color;	
	float fog = clamp(gl_Fog.scale*(gl_Fog.end + gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);

}
);

#if TECHNICAL_MODE
const char *grayVeinMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
	float noise3D(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f; 
	}

	float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
		
		float freq = 1.0f;
		float amp = 0.8f;
		float sum = 0.0f;
		
		for(int i=0; i<octaves; i++) {
			sum += abs(noise3D(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float spike(float c, float w, float x) {
		return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
	}

	vec3 myTexture3DCom(vec3 p, float mat)
	{
		
		
		float noiseScale = 0.05f * extraNoiseScale;
		float noise = turbulence(p*noiseScale, 8, 2.0f, 0.5f);
		
		noise = spike(0.35f, 0.05f, noise);
		noise = noise;

		vec3 base = lerp(vec3(1.0f,1.0f,1.0f), vec3(0.9f,0.9f,0.9f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
		vec3 b2 = lerp(base, vec3(0.8f, 0.8f, 0.8f), noise); 
	/*
		return lerp(b2, vec3(0.5f,0.5f,0.5f), 
			spike(0.4f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)) +
			spike(0.3f, 0.05f, turbulence(p*noiseScale*0.4f, 3, 5.0f, 0.5f))
			);
		*/
		return 0.6*lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
		//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

		//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
		//return vec3(1.0f,1.0f,noise);
		//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
	}
);

#else
const char *grayVeinMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
	float noise3D(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f; 
	}

	float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
		
		float freq = 1.0f;
		float amp = 0.8f;
		float sum = 0.0f;
		
		for(int i=0; i<octaves; i++) {
			sum += abs(noise3D(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float spike(float c, float w, float x) {
		return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
	}

	vec3 myTexture3DCom(vec3 p, float mat)
	{
		
		
		float noiseScale = 0.05f * extraNoiseScale;
		float noise = turbulence(p*noiseScale, 8, 2.0f, 0.5f);
		
		noise = spike(0.35f, 0.05f, noise);
		noise = noise;

		vec3 base = lerp(vec3(1.0f,1.0f,1.0f), vec3(0.9f,0.9f,0.9f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
		vec3 b2 = lerp(base, vec3(0.8f, 0.8f, 0.8f), noise); 
	/*
		return lerp(b2, vec3(0.5f,0.5f,0.5f), 
			spike(0.4f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)) +
			spike(0.3f, 0.05f, turbulence(p*noiseScale*0.4f, 3, 5.0f, 0.5f))
			);
		*/
		return lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
		//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

		//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
		//return vec3(1.0f,1.0f,noise);
		//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
	}
);

#endif
const char *sandStoneTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
float noise3D(vec3 p)
{
	return texture3D(ttt3D, p).x*2.0f - 1.0f; 
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {

	float freq = 1.0f;
	float amp = 0.8f;
	float sum = 0.0f;

	for(int i=0; i<octaves; i++) {
		sum += abs(noise3D(p*freq))*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float spike(float c, float w, float x) {
	return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
}

vec3 myTexture3DCom(vec3 p, float mat)
{


	float noiseScale = 0.1f*extraNoiseScale;
	float noise = turbulence(p*noiseScale, 3, 3.0f, 0.5f);
	//noise = turbulence(p*noiseScale + vec3(noise, noise, noise*0.3)*0.01f, 8, 3.0f, 0.5f);

	//noise = spike(0.35f, 0.05f, noise);
	//noise = noise;

	vec3 base = lerp(vec3(164,148,108)*1.63/255, vec3(178,156,126)*1.73/255, spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f + vec3(noise*0.5, noise, noise)*0.011f, 2, 2.0f, 0.5f)));
	//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
	vec3 b2 = lerp(base, vec3(173, 160, 121)*1.73/255, noise); 



	return b2;





}
);
const char *whiteMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
	float noise3D(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f; 
	}

	float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
		
		float freq = 1.0f;
		float amp = 0.8f;
		float sum = 0.0f;
		
		for(int i=0; i<octaves; i++) {
			sum += abs(noise3D(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float spike(float c, float w, float x) {
		return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
	}

	vec3 myTexture3DCom(vec3 p, float mat)
	{
		float noiseScale = 0.05f * extraNoiseScale;
		float noise = turbulence(p*noiseScale, 8, 2.0f, 0.5f);
		
		noise = spike(0.35f, 0.05f, noise);
		noise = noise;

		vec3 base = lerp(vec3(1.0f,1.0f,1.0f), vec3(0.99f,0.99f,0.99f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
		vec3 b2 = lerp(base, vec3(0.97f, 0.97f, 0.97f), noise); 
	/*
		return lerp(b2, vec3(0.5f,0.5f,0.5f), 
			spike(0.4f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)) +
			spike(0.3f, 0.05f, turbulence(p*noiseScale*0.4f, 3, 5.0f, 0.5f))
			);
		*/
		return lerp(b2, vec3(0.95f,0.95f,0.95f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
		//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

		//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
		//return vec3(1.0f,1.0f,noise);
		//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
	}
);

const char *blueVeinMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;

	float noise3D(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f; 
	}

	float turbulence(vec3 p, int octaves, float lacunarity, float gain) {
		
		float freq = 1.0f;
		float amp = 0.8f;
		float sum = 0.0f;
		
		for(int i=0; i<octaves; i++) {
			sum += abs(noise3D(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float spike(float c, float w, float x) {
		return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
	}

	vec3 myTexture3DCom(vec3 p, float mat)
	{
		
		
		float noiseScale = 0.05f*extraNoiseScale;
		float noise = turbulence(p*noiseScale, 8, 2.0f, 0.5f);
		
		noise = spike(0.35f, 0.05f, noise);
		noise = noise;

		vec3 base = lerp(vec3(1.0f,1.0f,1.0f), vec3(0.9f,0.9f,0.9f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
		vec3 b2 = lerp(base, vec3(0.7f, 0.7f, 0.9f), noise); 
	
		return lerp(b2, vec3(0.6f, 0.6f, 0.8f), 
			spike(0.4f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)) +
			spike(0.3f, 0.05f, turbulence(p*noiseScale*0.4f, 3, 5.0f, 0.5f))
			);
		
	//	return lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
		//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

		//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
		//return vec3(1.0f,1.0f,noise);
		//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
	}
);

const char *greenMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
float noise3D(vec3 p)
{
	return texture3D(ttt3D, p).x*2.0f - 1.0f; 
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {

	float freq = 1.0f;
	float amp = 0.8f;
	float sum = 0.0f;

	for(int i=0; i<octaves; i++) {
		sum += abs(noise3D(p*freq))*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float spike(float c, float w, float x) {
	return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
}

vec3 myTexture3DCom(vec3 p, float mat)
{


	float noiseScale = 0.05f*extraNoiseScale;
	float noise = turbulence(p*noiseScale, 8, 2.0f, 0.5f);

	noise = spike(0.35f, 0.05f, noise);
	noise = noise;

	vec3 base = lerp(vec3(0.2f,0.4f,0.2f), vec3(0.05f,0.3f,0.1f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
	//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
	vec3 b2 = lerp(base, vec3(0.5f, 0.5f, 0.5f), noise); 

	//return lerp(b2, vec3(0.1f, 0.05f, 0.05f), 
	return lerp(b2, vec3(0.2f, 0.3f, 0.2f), 
		spike(0.4f, 0.05f, turbulence(p*noiseScale, 4.3, 3.0f, 0.35f)) +
		spike(0.3f, 0.05f, turbulence(p*noiseScale*0.5f, 3, 5.0f, 0.5f))
		)*1.3f;

	//	return lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
	//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

	//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
	//return vec3(1.0f,1.0f,noise);
	//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
}
);

const char *redGraniteTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
float noise3D(vec3 p)
{
	return texture3D(ttt3D, p).x*2.0f - 1.0f; 
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {

	float freq = 1.0f;
	float amp = 0.8f;
	float sum = 0.0f;

	for(int i=0; i<octaves; i++) {
		sum += abs(noise3D(p*freq))*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float spike(float c, float w, float x) {
	return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
}

vec3 myTexture3DCom(vec3 p, float mat)
{


	float noiseScale = 0.3f*extraNoiseScale;
	float noise = turbulence(p*noiseScale, 8, 3.0f, 0.5f);

	//noise = spike(0.35f, 0.05f, noise);
	//noise = noise;

	vec3 base = lerp(vec3(0.75,0.3f,0.25f), vec3(0.1f,0.1f,0.1f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f, 4, 2.0f, 0.5f)));
	//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
	vec3 b2 = lerp(base, vec3(0.7f, 0.7f, 0.7f), noise); 

	return b2;
	//return lerp(b2, vec3(0.1f, 0.05f, 0.05f), 
/*	return lerp(b2, vec3(0.2f, 0.3f, 0.2f), 
		spike(0.4f, 0.05f, turbulence(p*noiseScale, 4.3, 3.0f, 0.35f)) +
		spike(0.3f, 0.05f, turbulence(p*noiseScale*0.5f, 3, 5.0f, 0.5f))
		)*1.3f;
		*/

	//	return lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
	//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

	//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
	//return vec3(1.0f,1.0f,noise);
	//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
}
);

const char *grayMarbleTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;	
float noise3D(vec3 p)
{
	return texture3D(ttt3D, p).x*2.0f - 1.0f; 
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {

	float freq = 1.0f;
	float amp = 0.8f;
	float sum = 0.0f;

	for(int i=0; i<octaves; i++) {
		sum += abs(noise3D(p*freq))*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float spike(float c, float w, float x) {
	return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
}

vec3 myTexture3DCom(vec3 p, float mat)
{


	float noiseScale = 0.05f*extraNoiseScale;
	float noise = turbulence(p*noiseScale, 8, 3.0f, 0.5f);
	noise = turbulence(p*noiseScale + vec3(noise, noise, noise)*0.015f, 8, 3.0f, 0.5f);

	//noise = spike(0.35f, 0.05f, noise);
	//noise = noise;

	vec3 base = lerp(vec3(0.85,0.85f,0.85f), vec3(0.65f,0.65f,0.65f), spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f + vec3(noise,2*noise, 0)*0.01f, 4, 2.0f, 0.5f)));
	//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
	vec3 b2 = lerp(base, vec3(0.3f, 0.3f, 0.3f), noise); 

	return b2;
	//return lerp(b2, vec3(0.1f, 0.05f, 0.05f), 
/*	return lerp(b2, vec3(0.2f, 0.3f, 0.2f), 
		spike(0.4f, 0.05f, turbulence(p*noiseScale, 4.3, 3.0f, 0.35f)) +
		spike(0.3f, 0.05f, turbulence(p*noiseScale*0.5f, 3, 5.0f, 0.5f))
		)*1.3f;
		*/

	//	return lerp(b2, vec3(0.7f,0.7f,0.7f), spike(0.35f, 0.05f, turbulence(p*noiseScale, 4, 3.0f, 0.35f)));
	//return lerp(b2, vec3(0.5f,0.5f,0.5f), turbulence(p*noiseScale, 4, 3.0f, 0.35f));

	//return lerp(base, vec3(0.1f, 0.1f, 0.4f), noise); 
	//return vec3(1.0f,1.0f,noise);
	//return lerp(vec3(1.0f,1.0f,0.7f), vec3(0.1f,0.1f,0.4f), noise);
}
);

const char *yellowGraniteTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	uniform float extraNoiseScale = 1.0f;
float noise3D(vec3 p)
{
	return texture3D(ttt3D, p).x*2.0f - 1.0f; 
}

float turbulence(vec3 p, int octaves, float lacunarity, float gain) {

	float freq = 1.0f;
	float amp = 0.8f;
	float sum = 0.0f;

	for(int i=0; i<octaves; i++) {
		sum += abs(noise3D(p*freq))*amp;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

float spike(float c, float w, float x) {
	return smoothstep(c-w, c, x) * smoothstep(c+w, c, x); 
}

vec3 myTexture3DCom(vec3 p, float mat)
{


	float noiseScale = 0.3f*extraNoiseScale;
	float noise = turbulence(p*noiseScale, 8, 3.0f, 0.5f);
	noise = turbulence(p*noiseScale + vec3(noise, noise, noise*0.3)*0.01f, 8, 3.0f, 0.5f);

	//noise = spike(0.35f, 0.05f, noise);
	//noise = noise;

	vec3 base = lerp(vec3(236,219,151)/255, vec3(179,145,28)/255, spike(0.5f, 0.3f, turbulence(p*noiseScale*0.7f + vec3(noise*0.5, noise, noise)*0.011f, 4, 2.0f, 0.5f)));
	//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
	vec3 b2 = lerp(base, vec3(0.1f, 0.1f, 0.1f), noise); 

	return b2;

}
);
/*
const char *woodTexture = STRINGIFY(
	uniform sampler3D ttt3D;
	float noise3D(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f; 
	}

	uniform vec3 woodColor1 = vec3(0.5,0.3, 0.1);
	uniform vec3 woodColor2 = vec3(0.25,0.15, 0.05);

	vec3 myTexture3DCom(vec3 p, float mat)
	{
		
		float noiseScale = 0.02f;
		float ampScale = 3.0f;
		float ringScale = 2.0f;
		
		p.z *= 0.01f;
		vec3 sp = p*noiseScale;
		vec3 noise3 = vec3(noise3D(sp), noise3D(sp+vec3(0.1,0.5,-0.2)), noise3D(sp+vec3(0.2,1.5,-0.3)));

		vec3 pwood = p + noise3*ampScale;
		vec3 r = ringScale * sqrt(dot(pwood.xz, pwood.xz));
		r = r + noise3D(sp+vec3(0.1,-1,2));
		r = r - floor(r);
		r = smoothstep(0.0, 0.8, r) - smoothstep(0.83,1,r);
		vec3 col = lerp(woodColor1, woodColor2, r);
		return col;
	}
);
*/
const char *woodTexture = STRINGIFY(
	uniform sampler3D ttt3D;

	uniform float extraNoiseScale = 1.0f;
	uniform float noiseScale = 0.03f;
	float noise(float p) {
		return texture3D(ttt3D, vec3(p*noiseScale*extraNoiseScale,0.5,0.5)).x;
	}

	float noise(float p, float q) {
		return texture3D(ttt3D, vec3(p*noiseScale*extraNoiseScale,q*noiseScale*extraNoiseScale,0.5)).x;
	}

	float snoise(float p) {
		return noise(p)*2.0f - 1.0f;
	}
	float snoise(float p, float q) {
		return noise(p,q)*2.0f - 1.0f;
	}
	float boxstep(float a, float b, float x) {
		return (clamp(((x)-(a))/((b)-(a)),0,1));

	}

	uniform float Ka = 1;
	uniform float Kd = 0.75;
	uniform float Ks = 0.15;
	uniform float roughness = 0.025;
    uniform vec3 specularcolor = vec3(1,1,1);
    uniform float ringscale = 0;
	uniform float grainscale = 0;
    uniform float txtscale = 1;
    uniform float plankspertile = 4;
    uniform vec3 lightwood = vec3(0.57, 0.292, 0.125);
    uniform vec3 darkwood  = vec3(0.275, 0.15, 0.06);
    uniform vec3 groovecolor  = vec3 (.05, .04, .015);
    //uniform float plankwidth = .05;
	uniform float plankwidth = .2;
	uniform float groovewidth = 0.001;
    uniform float plankvary = 0.8;
    uniform float grainy = 1;
	uniform float wavy = 0.08; 
	uniform float MINFILTERWIDTH = 1.0e-7;
	
	vec3 myTexture3DCom(vec3 p, float mat)
	{

	  float r;
	  float r2;
	  float whichrow;
	  float whichplank;
	  float swidth;
	  float twidth;
	  float fwidth;
	  float ss;
	  float tt;
	  float w;
	  float h;
	  float fade;
	  float ttt;
	  vec3 Ct;
	  vec3 woodcolor;
	  float groovy;
	  float PGWIDTH;
	  float PGHEIGHT;
	  float GWF;
	  float GHF;
	  float tilewidth;
	  float whichtile;
	  float tmp;
	  float planklength;

	  
	  PGWIDTH = plankwidth+groovewidth;
	  planklength = PGWIDTH * plankspertile - groovewidth;
	  
	  PGHEIGHT = planklength+groovewidth;
	  GWF = groovewidth*0.5/PGWIDTH;
	  GHF = groovewidth*0.5/PGHEIGHT;

	  // Determine how wide in s-t space one pixel projects to 
	  float s = p.x;
	  float t = p.y;
	  float du = 1.0;
	  float dv = 1.0;

	  swidth = (max (abs(dFdx(s)*du) + abs(dFdy(s)*dv), MINFILTERWIDTH) /
	PGWIDTH) * txtscale;
	  twidth = (max (abs(dFdx(t)*du) + abs(dFdy(t)*dv), MINFILTERWIDTH) /
	PGHEIGHT) * txtscale;
	  fwidth = max(swidth,twidth);

	  ss = (txtscale * s) / PGWIDTH;
	  whichrow = floor (ss);
	  tt = (txtscale * t) / PGHEIGHT;
	  whichplank = floor(tt);

	  if (mod (whichrow/plankspertile + whichplank, 2) >= 1) {
		  ss = txtscale * t / PGWIDTH;
		  whichrow = floor (ss);
		  tt = txtscale * s / PGHEIGHT;
		  whichplank = floor(tt);
		  tmp = swidth;  swidth = twidth;  twidth = tmp;
		} 
	  ss -= whichrow;
	  tt -= whichplank;
	  whichplank += 20*(whichrow+10);

	  if (swidth >= 1)
		  w = 1 - 2*GWF;
	  else w = clamp (boxstep(GWF-swidth,GWF,ss), max(1-GWF/swidth,0), 1)
	 - clamp (boxstep(1-GWF-swidth,1-GWF,ss), 0, 2*GWF/swidth);
	  if (twidth >= 1)
		  h = 1 - 2*GHF;
	  else h = clamp (boxstep(GHF-twidth,GHF,tt), max(1-GHF/twidth,0),1)
	 - clamp (boxstep(1-GHF-twidth,1-GHF,tt), 0, 2*GHF/twidth);
	  // This would be the non-antialiased version:
	   //w = step (GWF,ss) - step(1-GWF,ss);
	   //h = step (GHF,tt) - step(1-GHF,tt);
	   
	  groovy = w*h;


	  
	  // Add the ring patterns
	  fade = smoothstep (1/ringscale, 8/ringscale, fwidth);
	  if (fade < 0.999) {

		  ttt = tt/4+whichplank/28.38 + wavy * noise (8*ss, tt/4);
		  r = ringscale * noise (ss-whichplank, ttt);
		  r -= floor (r);
		  r = 0.3+0.7*smoothstep(0.2,0.55,r)*(1-smoothstep(0.75,0.8, r));
		  r = (1-fade)*r + 0.65*fade;
		  
		   // Multiply the ring pattern by the fine grain
		  
		  fade = smoothstep (2/grainscale, 8/grainscale, fwidth);
		  if (fade < 0.999) {
		 r2 = 1.3 - noise (ss*grainscale, (tt*grainscale/4));
		 r2 = grainy * r2*r2 + (1-grainy);
		 r *= (1-fade)*r2 + (0.75*fade);
		   
		}
		  else r *= 0.75;
		  
		}
	  else r = 0.4875;
	  

	  // Mix the light and dark wood according to the grain pattern 
	  woodcolor = lerp (lightwood, darkwood, r);

	  // Add plank-to-plank variation in overall color 
	  woodcolor *= (1-plankvary/2 + plankvary * noise (whichplank+0.5));

	  Ct = lerp (groovecolor, woodcolor, groovy);
	  return Ct;
	  
	}
);

const char *combineTexture = STRINGIFY(
	uniform sampler3D ttt3D;

	uniform float extraNoiseScale = 1.0f;
	uniform float noiseScale = 0.03f;
	float noise(float p) {
		return texture3D(ttt3D, vec3(p*noiseScale*extraNoiseScale, 0.5, 0.5)).x;
	}

	float noise(float p, float q) {
		return texture3D(ttt3D, vec3(p*noiseScale*extraNoiseScale, q*noiseScale*extraNoiseScale, 0.5)).x;
	}

	float snoise(float p) {
		return noise(p)*2.0f - 1.0f;
	}
	float snoise(float p, float q) {
		return noise(p, q)*2.0f - 1.0f;
	}
	float boxstep(float a, float b, float x) {
		return (clamp(((x)-(a)) / ((b)-(a)), 0, 1));

	}

	uniform float Ka = 1;
	uniform float Kd = 0.75;
	uniform float Ks = 0.15;
	uniform float roughness = 0.025;
	uniform vec3 specularcolor = vec3(1, 1, 1);
	uniform float ringscale = 0;
	uniform float grainscale = 0;
	uniform float txtscale = 1;
	uniform float plankspertile = 4;
	uniform vec3 lightwood = vec3(0.57, 0.292, 0.125);
	uniform vec3 darkwood = vec3(0.275, 0.15, 0.06);
	uniform vec3 groovecolor = vec3(.05, .04, .015);
	//uniform float plankwidth = .05;
	uniform float plankwidth = .2;
	uniform float groovewidth = 0.001;
	uniform float plankvary = 0.8;
	uniform float grainy = 1;
	uniform float wavy = 0.08;
	uniform float MINFILTERWIDTH = 1.0e-7;

	vec3 myTexture3D_0(vec3 p)
	{		
		float r;
		float r2;
		float whichrow;
		float whichplank;
		float swidth;
		float twidth;
		float fwidth;
		float ss;
		float tt;
		float w;
		float h;
		float fade;
		float ttt;
		vec3 Ct;
		vec3 woodcolor;
		float groovy;
		float PGWIDTH;
		float PGHEIGHT;
		float GWF;
		float GHF;
		float tilewidth;
		float whichtile;
		float tmp;
		float planklength;


		PGWIDTH = plankwidth + groovewidth;
		planklength = PGWIDTH * plankspertile - groovewidth;

		PGHEIGHT = planklength + groovewidth;
		GWF = groovewidth*0.5 / PGWIDTH;
		GHF = groovewidth*0.5 / PGHEIGHT;

		// Determine how wide in s-t space one pixel projects to 
		float s = p.x;
		float t = p.y;
		float du = 1.0;
		float dv = 1.0;

		swidth = (max(abs(dFdx(s)*du) + abs(dFdy(s)*dv), MINFILTERWIDTH) /
			PGWIDTH) * txtscale;
		twidth = (max(abs(dFdx(t)*du) + abs(dFdy(t)*dv), MINFILTERWIDTH) /
			PGHEIGHT) * txtscale;
		fwidth = max(swidth, twidth);

		ss = (txtscale * s) / PGWIDTH;
		whichrow = floor(ss);
		tt = (txtscale * t) / PGHEIGHT;
		whichplank = floor(tt);

		if (mod(whichrow / plankspertile + whichplank, 2) >= 1) {
			ss = txtscale * t / PGWIDTH;
			whichrow = floor(ss);
			tt = txtscale * s / PGHEIGHT;
			whichplank = floor(tt);
			tmp = swidth;  swidth = twidth;  twidth = tmp;
		}
		ss -= whichrow;
		tt -= whichplank;
		whichplank += 20 * (whichrow + 10);

		if (swidth >= 1)
			w = 1 - 2 * GWF;
		else w = clamp(boxstep(GWF - swidth, GWF, ss), max(1 - GWF / swidth, 0), 1)
			- clamp(boxstep(1 - GWF - swidth, 1 - GWF, ss), 0, 2 * GWF / swidth);
		if (twidth >= 1)
			h = 1 - 2 * GHF;
		else h = clamp(boxstep(GHF - twidth, GHF, tt), max(1 - GHF / twidth, 0), 1)
			- clamp(boxstep(1 - GHF - twidth, 1 - GHF, tt), 0, 2 * GHF / twidth);
		// This would be the non-antialiased version:
		//w = step (GWF,ss) - step(1-GWF,ss);
		//h = step (GHF,tt) - step(1-GHF,tt);

		groovy = w*h;



		// Add the ring patterns
		fade = smoothstep(1 / ringscale, 8 / ringscale, fwidth);
		if (fade < 0.999) {

			ttt = tt / 4 + whichplank / 28.38 + wavy * noise(8 * ss, tt / 4);
			r = ringscale * noise(ss - whichplank, ttt);
			r -= floor(r);
			r = 0.3 + 0.7*smoothstep(0.2, 0.55, r)*(1 - smoothstep(0.75, 0.8, r));
			r = (1 - fade)*r + 0.65*fade;

			// Multiply the ring pattern by the fine grain

			fade = smoothstep(2 / grainscale, 8 / grainscale, fwidth);
			if (fade < 0.999) {
				r2 = 1.3 - noise(ss*grainscale, (tt*grainscale / 4));
				r2 = grainy * r2*r2 + (1 - grainy);
				r *= (1 - fade)*r2 + (0.75*fade);

			}
			else r *= 0.75;

		}
		else r = 0.4875;


		// Mix the light and dark wood according to the grain pattern 
		woodcolor = lerp(lightwood, darkwood, r);

		// Add plank-to-plank variation in overall color 
		woodcolor *= (1 - plankvary / 2 + plankvary * noise(whichplank + 0.5));

		Ct = lerp(groovecolor, woodcolor, groovy);
		return Ct;

	}

	float noise3D_1(vec3 p)
	{
		return texture3D(ttt3D, p).x*2.0f - 1.0f;
	}

	float turbulence_1(vec3 p, int octaves, float lacunarity, float gain) {

		float freq = 1.0f;
		float amp = 0.8f;
		float sum = 0.0f;

		for (int i = 0; i<octaves; i++) {
			sum += abs(noise3D_1(p*freq))*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}

	float spike_1(float c, float w, float x) {
		return smoothstep(c - w, c, x) * smoothstep(c + w, c, x);
	}

	vec3 myTexture3D_1(vec3 p)
	{


		float noiseScale = 0.1f*extraNoiseScale;
		float noise = turbulence_1(p*noiseScale, 3, 3.0f, 0.5f);
		//noise = turbulence(p*noiseScale + vec3(noise, noise, noise*0.3)*0.01f, 8, 3.0f, 0.5f);

		//noise = spike(0.35f, 0.05f, noise);
		//noise = noise;

		vec3 base = lerp(vec3(164, 148, 108)*1.63 / 255, vec3(178, 156, 126)*1.73 / 255, spike_1(0.5f, 0.3f, turbulence_1(p*noiseScale*0.7f + vec3(noise*0.5, noise, noise)*0.011f, 2, 2.0f, 0.5f)));
		//vec3 b2 = lerp(base, vec3(0.0f, 0.0f, 0.0f), noise); 
		vec3 b2 = lerp(base, vec3(173, 160, 121)*1.73 / 255, noise);



		return b2*0.75f;

	}

	vec3 myTexture3DCom(vec3 p, float mat) {
		// Depend on material ID
		if (mat < 0.5f) {
			return vec3(173, 160, 151) *0.85 / 255;
			//return darkwood;
			//return vec3(0.75f, 0.75f, 0.75f);
		}
		else
			if (mat < 1.5f) {
				//return vec3(173, 160, 121)*1.73 / 255;
				return vec3(173, 100, 21)*1.73 / 255;
			}
			else {
				return vec3(1.0f, 0.0f, 0.0f);

			}
			/*
		if (mat < 0.5f) {
			return myTexture3D_0(p);
		}
		else 
		if (mat < 1.5f) {
			return myTexture3D_1(p);
		} else {
			return vec3(1.0f, 0.0f, 0.0f);
			
		}
		*/
	}

);

#if TECHNICAL_MODE
const char *shadowDiffuse3DFragmentProgram = STRINGIFY(
	// scene reflection 
	uniform float reflectionCoeff = 0.0f;
uniform float specularCoeff = 0.0f;

uniform sampler2DRect reflectionTex;

// Shadow map
uniform float shadowAmbient = 0.0;
uniform float hdrScale = 5.0;
uniform sampler2D texture;
uniform sampler2DArrayShadow stex;
uniform sampler2DArrayShadow stex2;
uniform sampler2DArrayShadow stex3;
uniform samplerCube skyboxTex;
uniform vec2 texSize; // x - size, y - 1/size
uniform vec4 far_d;

// Spot lights
uniform vec3 spotLightDir;
uniform vec3 spotLightPos;
uniform float spotLightCosineDecayBegin;
uniform float spotLightCosineDecayEnd;

uniform vec3 spotLightDir2;
uniform vec3 spotLightPos2;
uniform float spotLightCosineDecayBegin2;
uniform float spotLightCosineDecayEnd2;

uniform vec3 spotLightDir3;
uniform vec3 spotLightPos3;
uniform float spotLightCosineDecayBegin3;
uniform float spotLightCosineDecayEnd3;

uniform vec3 parallelLightDir;
uniform float shadowAdd;
uniform int useTexture;
uniform int numShadows;


uniform float roughnessScale;
uniform vec3 ambientColor;

uniform sampler2DArray diffuseTexArray;
uniform sampler2DArray bumpTexArray;
uniform sampler2DArray specularTexArray;
uniform sampler2DArray emissiveReflectSpecPowerTexArray;

uniform vec2 shadowTaps[12];


float shadowCoeff1()
{

	int index = 3;

	if (gl_FragCoord.z < far_d.x)
		index = 0;
	else if (gl_FragCoord.z < far_d.y)
		index = 1;
	else if (gl_FragCoord.z < far_d.z)
		index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);


	// Gaussian 3x3 filter
	//	return shadow2DArray(stex, shadow_coord).x;
	/*
	const float X = 1.0f;
	float ret = shadow2DArray(stex, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;*/
	const int numTaps = 4;
	float radius = 0.0005f / pow(2, index);
	float s = 0.0f;
	for (int i = 0; i < numTaps; i++)
	{
		s += shadow2DArray(stex, shadow_coord + vec4(shadowTaps[i] * radius, 0.0f, 0.0f)).r;
	}
	s /= numTaps;
	return s;
}
float shadowCoeff2()
{
	const int index = 1;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);
	//	return shadow2DArray(stex, shadow_coord).x;

	const float X = 1.0f;
	float ret = shadow2DArray(stex2, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(-X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(-X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(-X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2(X, X)).x * 0.0625;
	return ret;
}
float shadowCoeff3()
{
	const int index = 2;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index] * vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);

	//	return shadow2DArray(stex, shadow_coord).x;

	const float X = 1.0f;
	float ret = shadow2DArray(stex3, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(-X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(-X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(-X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2(X, X)).x * 0.0625;
	return ret;
}



uniform float RollOff = 0.5f;
uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 3.0;   // 5.0 is physically correct
void main()
{

	/*
	int index = 3;

	if(gl_FragCoord.z < far_d.x)
	index = 0;
	else if(gl_FragCoord.z < far_d.y)
	index = 1;
	else if(gl_FragCoord.z < far_d.z)
	index = 2;
	if (index == 3) gl_FragColor = vec4(1,0,0,1);
	if (index == 2) gl_FragColor = vec4(0,1,0,1);
	if (index == 1) gl_FragColor = vec4(0,0,1,1);
	if (index == 0) gl_FragColor = vec4(1,1,0,1);
	return;*/
	/*
	int index = 3;

	if(gl_FragCoord.z < far_d.x)
	index = 0;
	else if(gl_FragCoord.z < far_d.y)
	index = 1;
	else if(gl_FragCoord.z < far_d.z)
	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index)*0.33333333f;
	gl_FragColor = vec4(shadow_coord.xyz,1.0f);
	return;
	*/
	//// TODO, expose this as user parameter
	const float skyLightIntensity = 0.2;
	const float rimLightIntensity = 0.3;

	vec3 normal = normalize(gl_TexCoord[2].xyz);
	vec3 t0 = gl_TexCoord[3].xyz;
	vec3 t1 = gl_TexCoord[4].xyz;

	vec3 diffuseMat;
	vec3 specularMat;
	vec3 bump;
	vec3 emissiveReflectSpecPow;

	// read in material color for diffuse, specular, bump, emmisive

	// 3D texture	
	diffuseMat = myTexture3DCom(gl_TexCoord[0].xyz, gl_TexCoord[6].w);
	//diffuseMat = myTexture3D(gl_TexCoord[0].xyz);//texture3D(ttt3D, gl_TexCoord[0].xyz);
	//diffuseMat = texture3D(ttt3D, gl_TexCoord[0].xyz);

	specularMat = vec3(1.0);
	bump = texture2D(texture, gl_TexCoord[5].xy).xyz;
	if (dot(bump, bump) < 0.01) bump = vec3(0.5, 0.5, 1);
	emissiveReflectSpecPow = vec3(0.0, 0.0, 0.0);


	// apply bump to the normal
	bump = (bump - vec3(0.5, 0.5, 0.5)) * 2.0f;
	bump.xy *= roughnessScale*0.1;

	float sc = 1.0f;
	normal = normalize(t0*bump.x + t1*bump.y + sc*normal * bump.z);

	//gl_FragColor.xyz = normal*0.5 + vec3(0.5,0.5,0.5);
	//gl_FragColor.w = 1;
	//return;
	vec3 eyeVec = normalize(gl_TexCoord[1].xyz);

	// apply gamma correction for diffuse textures
	//diffuseMat = pow(diffuseMat, 0.45);

	float specularPower = emissiveReflectSpecPow.b*255.0f + 1.0f;

	// TODO - fix this
	specularPower = 10.0f;

	float emissive = 0.0f;
	float reflectivity = emissiveReflectSpecPow.b;
	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(normal, eyeVec)), fresnelPower);
	float specular = 0.0f;

	vec3 skyNormal = reflect(eyeVec, normal);
	vec3 skyColor = skyLightIntensity * textureCube(skyboxTex, skyNormal).rgb;
	vec3 ambientSkyColor = diffuseMat * skyColor;

	vec3 diffuseColor = vec3(0.0, 0.0, 0.0);

	if (numShadows >= 1) {

		vec3 lightColor = hdrScale * vec3(1.0, 1.0, 1.0);
		vec3 shadowColor = vec3(0.4, 0.4, 0.7); // colored shadow
		//vec3 lvec = normalize(spotLightDir);
		vec3 lvec = normalize(spotLightPos - gl_TexCoord[1].xyz);
		float cosine = dot(lvec, spotLightDir);
		float intensity = smoothstep(spotLightCosineDecayBegin, spotLightCosineDecayEnd, cosine);

		float ldn = max(0.0f, dot(normal, lvec));
		float shadowC = shadowCoeff1();
		//gl_FragColor = vec4(shadowC,shadowC,shadowC,1.0f);
		//return;
		vec3 irradiance = shadowC * ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance*intensity;

		// add colored shadow
		diffuseColor += (1.0 - shadowC) * shadowAmbient * shadowColor * diffuseMat*intensity;

		vec3 r = reflect(lvec, normal);	
		specular += pow(max(0.0, dot(r, eyeVec)), specularPower)*shadowC*intensity;
	}

	// add rim light
	if (numShadows >= 2) {
		vec3 lightColor = hdrScale * vec3(1.0, 1.0, 1.0);
		vec3 lvec = normalize(spotLightDir2);
		float ldn = max(0.0f, dot(normal, lvec));
		vec3 irradiance = ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance;
	}

	vec3 color = vec3(0.0, 0.0, 0.0);

	color += diffuseColor;
	color += ambientSkyColor;
	color += specular*specularMat;
	color += hdrScale * emissive * diffuseMat;

	//vec3 reflectColor = diffuseMat * texture2DRect(reflectionTex, gl_FragCoord.xy).rgb;
	//color = reflectionCoeff * reflectColor + (1.0f - reflectionCoeff) * color;
	color = (fresnel * skyColor + (1.0 - fresnel) * color) * reflectivity + (1.0 - reflectivity) * color;

	gl_FragColor.rgb = color;
	gl_FragColor.w = gl_Color.w;

	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);
}
);
#else
const char *shadowDiffuse3DFragmentProgram = STRINGIFY(
	// scene reflection 
	uniform float reflectionCoeff = 0.0f;
uniform float specularCoeff = 0.0f;

uniform sampler2DRect reflectionTex;

// Shadow map
uniform float shadowAmbient = 0.0;
uniform float hdrScale = 5.0;
uniform sampler2D texture;
uniform sampler2DArrayShadow stex;
uniform sampler2DArrayShadow stex2;
uniform sampler2DArrayShadow stex3;
uniform samplerCube skyboxTex;
uniform vec2 texSize; // x - size, y - 1/size
uniform vec4 far_d;

// Spot lights
uniform vec3 spotLightDir;
uniform vec3 spotLightPos;
uniform float spotLightCosineDecayBegin;
uniform float spotLightCosineDecayEnd;

uniform vec3 spotLightDir2;
uniform vec3 spotLightPos2;
uniform float spotLightCosineDecayBegin2;
uniform float spotLightCosineDecayEnd2;

uniform vec3 spotLightDir3;
uniform vec3 spotLightPos3;
uniform float spotLightCosineDecayBegin3;
uniform float spotLightCosineDecayEnd3;

uniform vec3 parallelLightDir;
uniform float shadowAdd;
uniform int useTexture;
uniform int numShadows;


uniform float roughnessScale;
uniform vec3 ambientColor;

uniform sampler2DArray diffuseTexArray;
uniform sampler2DArray bumpTexArray;
uniform sampler2DArray specularTexArray;
uniform sampler2DArray emissiveReflectSpecPowerTexArray;



float shadowCoeff1()
{
	const int index = 0;

	//int index = 3;
	//
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);


	// Gaussian 3x3 filter
	//	return shadow2DArray(stex, shadow_coord).x;
	const float X = 1.0f;
	float ret = shadow2DArray(stex, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex, shadow_coord, ivec2( X, X)).x * 0.0625;	
	return ret;
}
float shadowCoeff2()
{
	const int index = 1;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);
	//	return shadow2DArray(stex, shadow_coord).x;

	const float X = 1.0f;
	float ret = shadow2DArray(stex2, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex2, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;
}
float shadowCoeff3()
{
	const int index = 2;

	//int index = 3;
	//if(gl_FragCoord.z < far_d.x)
	//	index = 0;
	//else if(gl_FragCoord.z < far_d.y)
	//	index = 1;
	//else if(gl_FragCoord.z < far_d.z)
	//	index = 2;

	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd;
	shadow_coord.z = float(0);

	//	return shadow2DArray(stex, shadow_coord).x;

	const float X = 1.0f;
	float ret = shadow2DArray(stex3, shadow_coord).x * 0.25;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( -X, X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( 0, -X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( 0, X)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, -X)).x * 0.0625;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, 0)).x * 0.125;
	ret += shadow2DArrayOffset(stex3, shadow_coord, ivec2( X, X)).x * 0.0625;
	return ret;
}



uniform float RollOff = 0.5f;
uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 3.0;   // 5.0 is physically correct
void main()
{	
	//// TODO, expose this as user parameter
	const float skyLightIntensity = 0.2;
	const float rimLightIntensity = 0.3;

	vec3 normal = normalize(gl_TexCoord[2].xyz);
	vec3 t0 = gl_TexCoord[3].xyz;
	vec3 t1 = gl_TexCoord[4].xyz;

	vec3 diffuseMat;
	vec3 specularMat;
	vec3 bump;
	vec3 emissiveReflectSpecPow;

	// read in material color for diffuse, specular, bump, emmisive
	if (gl_TexCoord[6].z >= 0.0f) {
		// 2D texture
		diffuseMat = texture2DArray(diffuseTexArray, gl_TexCoord[6].xyz).rgb;
		//specularMat = texture2DArray(specularTexArray, gl_TexCoord[6].xyz).rgb; // TODO Does not seem to work
		specularMat = vec3(1.0f);
		bump = texture2DArray(bumpTexArray, gl_TexCoord[6].xyz).xyz;
		if (dot(bump,bump) < 0.01) bump = vec3(0.5,0.5,1);	
		emissiveReflectSpecPow = texture2DArray(emissiveReflectSpecPowerTexArray, gl_TexCoord[6].xyz).xyz;

	} else {
		// 3D texture
		diffuseMat = myTexture3DCom(gl_TexCoord[0].xyz,0.0f) * vec3(0.5,0.5,0.5);//texture3D(ttt3D, gl_TexCoord[0].xyz);
		specularMat = vec3(1.0);
		bump = texture2D(texture, gl_TexCoord[5].xy).xyz;
		if (dot(bump,bump) < 0.01) bump = vec3(0.5,0.5,1);		
		emissiveReflectSpecPow = vec3(0.0,0.0,0.0);
	}

	// apply bump to the normal
	bump = (bump - vec3(0.5,0.5,0.5)) * 2.0f;
	bump.xy *= roughnessScale*2;	

	float sc = 1.0f;
	normal = normalize(t0*bump.x + t1*bump.y + sc*normal * bump.z);

	//gl_FragColor.xyz = normal*0.5 + vec3(0.5,0.5,0.5);
	//gl_FragColor.w = 1;
	//return;
	vec3 eyeVec = normalize(gl_TexCoord[1].xyz);

	// apply gamma correction for diffuse textures
	diffuseMat.x = pow(diffuseMat.x, 0.45);
	diffuseMat.y = pow(diffuseMat.y, 0.45);
	diffuseMat.z = pow(diffuseMat.z, 0.45);

	float specularPower = emissiveReflectSpecPow.b*255.0f + 1.0f;

	// TODO - fix this
	specularPower = 10.0f;

	float emissive = emissiveReflectSpecPow.r*10.0f;
	float reflectivity = emissiveReflectSpecPow.b;
	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(normal, eyeVec)), fresnelPower);
	float specular = 0.0f;

	vec3 skyNormal = reflect(eyeVec, normal);
	vec3 skyColor = skyLightIntensity * textureCube(skyboxTex, skyNormal).rgb;
	vec3 ambientSkyColor = diffuseMat * skyColor;

	vec3 diffuseColor = vec3(0.0, 0.0, 0.0);	

	if (numShadows >= 1) {
		vec3 lightColor = hdrScale * vec3(1.0, 0.9, 0.9);
		vec3 shadowColor = vec3(0.4, 0.4, 0.9); // colored shadow
		vec3 lvec = normalize(spotLightDir);
		float ldn = max(0.0f, dot(normal, lvec));
		float shadowC = shadowCoeff1();
		vec3 irradiance = shadowC * ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance;

		// add colored shadow
		diffuseColor += (1.0 - shadowC) * shadowAmbient * shadowColor * diffuseMat;

		vec3 r = reflect(lvec, normal);	
		specular += pow(max(0.0, dot(r,eyeVec)), specularPower)*shadowC;
	}

	// add rim light
	if (numShadows >= 2) {
		vec3 lightColor = rimLightIntensity * vec3(1.0, 0.9, 0.9);
		vec3 lvec = normalize(spotLightDir2);
		float ldn = max(0.0f, dot(normal, lvec));
		vec3 irradiance = ldn * lightColor;

		// diffuse irradiance
		diffuseColor += diffuseMat * irradiance;
	}

	vec3 color = vec3(0.0, 0.0, 0.0);

	color += diffuseColor;
	color += ambientSkyColor;
	color += specular*specularMat;
	color += hdrScale * emissive * diffuseMat;

	//vec3 reflectColor = diffuseMat * texture2DRect(reflectionTex, gl_FragCoord.xy).rgb;
	//color = reflectionCoeff * reflectColor + (1.0f - reflectionCoeff) * color;
	color = (fresnel * skyColor + (1.0 - fresnel) * color) * reflectivity + (1.0 - reflectivity) * color;

	gl_FragColor.rgb = color;
	gl_FragColor.w = gl_Color.w;

	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);
}
);
#endif
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
const char *whiteFragmentProgram = STRINGIFY(
void main()
{
	gl_FragColor = vec4(1.0,1.0,1.0,1.0);
}
);

const char *normalFragmentProgram = STRINGIFY(
void main()
{
	gl_FragColor = vec4(normalize(gl_TexCoord[2].xyz) * 0.5 + 0.5, 1.0);
}
);

ShaderShadow::ShaderShadow(VS_MODE vsMode, PS_MODE psMode, SHADER3D_CHOICES shade3D)  : vsMode(vsMode), psMode(psMode), myShade3DChoice(shade3D)
{
	for (int i = 0; i < mNumLights; i++)
		mShadowMaps[i] = NULL;
	mReflectionTexId = 0;

	for (int i = 0; i < 16; i++) {
		mCamModelView[i] = i%5 == 0 ? 1.0f : 0.0f;
		mCamProj[i]      = i%5 == 0 ? 1.0f : 0.0f;
	}
	mNumShadows = mNumLights;
	mShowReflection = false;
	shadowAmbient = 0.2f;
	//ambientColor = 0.2f;
	ambientColor = PxVec3(0.1f, 0.1f, 0.2f);

	switch (shade3D) {
		case SAND_STONE: fragmentShader3DComposite = std::string(sandStoneTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(195,179,146)*1.23f/255.0f; break;
		case GRAYVEIN_MARBLE: fragmentShader3DComposite = std::string(grayVeinMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(1.0f,1.0f,1.0f); break;
		case WHITE_MARBLE: fragmentShader3DComposite = std::string(whiteMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(1.0f,1.0f,1.0f); break;
		case BLUEVEIN_MARBLE: fragmentShader3DComposite = std::string(blueVeinMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(1.0f,1.0f,1.0f); break;
		case GREEN_MARBLE: fragmentShader3DComposite = std::string(greenMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(0.3f,0.5f,0.3f); break;
		case GRAY_MARBLE: fragmentShader3DComposite = std::string(grayMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(1.0f,1.0f,1.0f); break;
		case RED_GRANITE: fragmentShader3DComposite = std::string(redGraniteTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(0.7f,0.45f,0.4f); break;
		case YELLOW_GRANITE: fragmentShader3DComposite = std::string(yellowGraniteTexture) + std::string(shadowDiffuse3DFragmentProgram); dustColor = PxVec3(236,219,151)/255.0f; break;
		case WOOD: fragmentShader3DComposite = std::string(woodTexture) + std::string(shadowDiffuse3DFragmentProgram);	dustColor = PxVec3(156,90,60)/255.0f; break;
		case COMBINE: fragmentShader3DComposite = std::string(combineTexture) + std::string(shadowDiffuse3DFragmentProgram);	dustColor = PxVec3(156, 90, 60) / 255.0f; break;
	}
	
	whiteShader = NULL;
	normalShader = NULL;
	if ((psMode != PS_WHITE) && (psMode != PS_NORMAL)) {
		whiteShader = new ShaderShadow(vsMode, PS_WHITE, shade3D);
		normalShader = new ShaderShadow(vsMode, PS_NORMAL, shade3D);
	}
		
	
		

	/*
	if (shade3D == GRAY_MARBLE) {
		fragmentShader3DComposite = std::string(grayMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram);
	} else 
	if (shade3D == BLUE_MARBLE) {
		fragmentShader3DComposite = std::string(blueMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram);
	} else {
		fragmentShader3DComposite = std::string(grayMarbleTexture) + std::string(shadowDiffuse3DFragmentProgram);
	}
*/
//	
	//fragmentShader3DComposite = std::string(yellowGraniteTexture) + std::string(shadowDiffuse3DFragmentProgram);

}

// --------------------------------------------------------------------------------------------
bool ShaderShadow::init()
{
	for (int i = 0; i < mNumLights; i++)
		mShadowMaps[i] = NULL;

	// some defaults
	for (int i = 0; i < mNumLights; i++) {
		mSpotLightCosineDecayBegin[i] = 0.98f;
		mSpotLightCosineDecayEnd[i] = 0.997f;
		mSpotLightPos[i] = PxVec3(10.0f, 10.0f, 10.0f);
		mSpotLightDir[i] = PxVec3(0.0f, 0.0f, 0.0f) - mSpotLightPos[i];
		mSpotLightDir[i].normalize();
	}

	mBackLightDir = PxVec3(0, 20.0f, 20.0f);
	const char* vsProg = 0;
	const char* psProg = 0;
	if (vsMode == VS_DEFAULT) vsProg = shadowDiffuseVertexProgram;
	if (vsMode == VS_4BONES) vsProg = shadowDiffuseVertexProgram4Bones;
	if (vsMode == VS_8BONES) vsProg = shadowDiffuseVertexProgram8Bones;
	if (vsMode == VS_INSTANCE) vsProg = shadowDiffuseVertexProgramInstance;	
	if (vsMode == VS_TEXINSTANCE) vsProg = shadowDiffuseVertexProgramTexInstance;
	if (vsMode == VS_DEFAULT_FOR_3D_TEX) vsProg = shadowDiffuseVertexProgramFor3DTex;
	if (vsMode == VS_CROSSHAIR) vsProg = crossHairVertexProgram;


	if (psMode == PS_SHADE3D) psProg = fragmentShader3DComposite.c_str();
	if (psMode == PS_SHADE) psProg = shadowDiffuseFragmentProgram;
	if (psMode == PS_WHITE) psProg = whiteFragmentProgram;
	if (psMode == PS_NORMAL) psProg = normalFragmentProgram;	
	if (psMode == PS_CROSSHAIR) psProg = crossHairFragmentProgram;
	
	
/*	if (vsMode == VS_4BONES) {
		if (!loadShaderCode(shadowDiffuseVertexProgram4Bones, shadowDiffuseFragmentProgram))
			return false;

	} else {
		if (!loadShaderCode(shadowDiffuseVertexProgram, shadowDiffuseFragmentProgram))
			return false;
	}*/
	if (!loadShaderCode(vsProg, psProg))
		return false;

	if ((psMode != PS_WHITE) && (psMode != PS_NORMAL)) {
		whiteShader->init();
		normalShader->init();
	}

	return true;
}

// --------------------------------------------------------------------------------------------
void ShaderShadow::setSpotLight(int nr, const PxVec3 &pos, PxVec3 &dir, float decayBegin, float decayEnd)
{
	if (nr < 0 || nr >= mNumLights)
		return;

	mSpotLightPos[nr] = pos;
	mSpotLightDir[nr] = dir;

	mSpotLightCosineDecayBegin[nr] = decayBegin;
	mSpotLightCosineDecayEnd[nr] = decayEnd;
}

// --------------------------------------------------------------------------------------------
void ShaderShadow::updateCamera(float* modelView, float* proj)
{
	for (int i = 0; i < 16; i++) {
		mCamModelView[i] = modelView[i];
		mCamProj[i] = proj[i];
	}
}

// --------------------------------------------------------------------------------------------
void ShaderShadow::setShadowAmbient(float sa) {
	shadowAmbient = sa;
}

// --------------------------------------------------------------------------------------------
void ShaderShadow::activate(const ShaderMaterial &mat)
{
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		whiteShader->activate(mat);
		return;
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		normalShader->activate(mat);
		return;
	}
	Shader::activate(mat);
	if ((psMode == PS_WHITE) || (psMode == PS_NORMAL)) return;

	for (int i = 0; i < mNumShadows; i++) {
		if (mShadowMaps[i] == NULL)
			return;
	}

	// surface texture
	setUniform("texture", 0);
	if (mat.texId > 0) {
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mat.texId);
		setUniform("useTexture", 1);
	}
	else
		setUniform("useTexture", 0);

	setUniform("numShadows", mNumShadows);

	glColor4fv(mat.color);

	// the three shadow maps
	if (mShadowMaps[0] != NULL) {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mShadowMaps[0]->getDepthTexArray());
		glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

		int size = mShadowMaps[0]->getTextureSize();
		setUniform2("texSize", (float)size, 1.0f /size);
		setUniform1("specularCoeff", mat.specularCoeff);
	}
	setUniform("stex", 2);

	if (mNumLights > 1) {
		if (mShadowMaps[1] != NULL) {
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mShadowMaps[1]->getDepthTexArray());
			glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		}
		setUniform("stex2", 3);
	}

	if (mNumLights > 2) {
		if (mShadowMaps[2] != NULL) {
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mShadowMaps[2]->getDepthTexArray());
			glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		}
		setUniform("stex3", 4);
	}

	glActiveTexture(GL_TEXTURE0);
	setUniform1("shadowAdd", g_shadowAdd);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mReflectionTexId);
	setUniform("reflectionTex", 5);
	if (mShowReflection)
		setUniform1("reflectionCoeff", mat.reflectionCoeff);
	else
		setUniform1("reflectionCoeff", 0.0f);

	if (ShaderShadow::skyBoxTex > 0)
	{
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, ShaderShadow::skyBoxTex);
		setUniform("skyboxTex", 6);
	}

	PxMat44 camTrans(mCamModelView);
	PxVec3 eyePos, eyeDir;

	eyePos = camTrans.transform(mSpotLightPos[0]);
	eyeDir = camTrans.rotate(mSpotLightDir[0]); eyeDir.normalize();
	setUniform3("spotLightDir", eyeDir.x, eyeDir.y, eyeDir.z);
	setUniform3("spotLightPos", eyePos.x, eyePos.y, eyePos.z);
	setUniform1("spotLightCosineDecayBegin", mSpotLightCosineDecayBegin[0]);
	setUniform1("spotLightCosineDecayEnd", mSpotLightCosineDecayEnd[0]);

	eyePos = camTrans.transform(mSpotLightPos[1]);
	eyeDir = camTrans.rotate(mSpotLightDir[1]); eyeDir.normalize();
	setUniform3("spotLightDir2", eyeDir.x, eyeDir.y, eyeDir.z);
	setUniform3("spotLightPos2", eyePos.x, eyePos.y, eyePos.z);
	setUniform1("spotLightCosineDecayBegin2", mSpotLightCosineDecayBegin[1]);
	setUniform1("spotLightCosineDecayEnd2", mSpotLightCosineDecayEnd[1]);

	eyePos = camTrans.transform(mSpotLightPos[2]);
	eyeDir = camTrans.rotate(mSpotLightDir[2]); eyeDir.normalize();
	setUniform3("spotLightDir3", eyeDir.x, eyeDir.y, eyeDir.z);
	setUniform3("spotLightPos3", eyePos.x, eyePos.y, eyePos.z);
	setUniform1("spotLightCosineDecayBegin3", mSpotLightCosineDecayBegin[2]);
	setUniform1("spotLightCosineDecayEnd3", mSpotLightCosineDecayEnd[2]);

	eyeDir = camTrans.rotate(mBackLightDir); eyeDir.normalize();
	setUniform3("parallelLightDir", eyeDir.x, eyeDir.y, eyeDir.z);

	if (mShadowMaps[0] != NULL) {
		setUniform4("far_d", 
			mShadowMaps[0]->getFarBound(0), 
			mShadowMaps[0]->getFarBound(1), 
			mShadowMaps[0]->getFarBound(2), 
			mShadowMaps[0]->getFarBound(3));
	}

	for (int i = 0; i < mNumLights; i++) {
		if (mShadowMaps[i] != NULL)
			mShadowMaps[i]->prepareForRender(mCamModelView, mCamProj);
	}
	setUniform1("shadowAmbient", shadowAmbient);
	setUniform1("hdrScale", hdrScale);
	setUniform3("ambientColor", ambientColor.x, ambientColor.y, ambientColor.z);

	float taps[] =
	{
		-0.326212f, -0.40581f, -0.840144f, -0.07358f,
		-0.695914f, 0.457137f, -0.203345f, 0.620716f,
		0.96234f, -0.194983f, 0.473434f, -0.480026f,
		0.519456f, 0.767022f, 0.185461f, -0.893124f,
		0.507431f, 0.064425f, 0.89642f, 0.412458f,
		-0.32194f, -0.932615f, -0.791559f, -0.59771f
	};
	setUniformfv("shadowTaps", &taps[0], 2, 12);
	
}


// --------------------------------------------------------------------------------------------
void ShaderShadow::deactivate()
{
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		whiteShader->deactivate();
		return;
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		normalShader->deactivate();
		return;
	}
	Shader::deactivate();
	if ((psMode == PS_WHITE) || (psMode == PS_NORMAL)) return;

	for (int i = 0; i < mNumLights; i++) {
		if (mShadowMaps[i] != NULL)
			mShadowMaps[i]->doneRender();
	}


	//glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_NORMAL_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE4);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
}

#include <fstream>
#include <string>
using namespace std;
static std::string LoadStringFromFile(const char* fname) {
	ifstream fin(fname);
	string line;
	string sss = "";
	while (getline(fin, line)) {
		sss += line;
		sss += "\n";
	}
	fin.close();
	return sss;
}

#include <direct.h>
#include <direct.h>
#define GetCurrentDir _getcwd


// --------------------------------------------------------------------------------------------
//bool ShaderShadow::forceLoadShaderFromFile() {
bool ShaderShadow::forceLoadShaderFromFile(VS_MODE vsMode, PS_MODE psMode, const char* vsName, const char* psName) {

	const char* vsProg = 0;
	const char* psProg = 0;
	if (vsMode == VS_DEFAULT) vsProg = shadowDiffuseVertexProgram;
	if (vsMode == VS_4BONES) vsProg = shadowDiffuseVertexProgram4Bones;
	if (vsMode == VS_8BONES) vsProg = shadowDiffuseVertexProgram8Bones;
	if (vsMode == VS_INSTANCE) vsProg = shadowDiffuseVertexProgramInstance;
	if (vsMode == VS_TEXINSTANCE) vsProg = shadowDiffuseVertexProgramTexInstance;


	if (psMode == PS_SHADE) psProg = shadowDiffuseFragmentProgram;
	if (psMode == PS_SHADE3D) psProg = fragmentShader3DComposite.c_str();
	if (psMode == PS_WHITE) psProg = whiteFragmentProgram;
	if (psMode == PS_NORMAL) psProg = normalFragmentProgram;

	std::string vsF;
	std::string psF;

	if (vsMode == VS_FILE) {
		vsF = LoadStringFromFile(vsName);
		vsProg = vsF.c_str();
	}

	if (psMode == PS_FILE) {
		psF = LoadStringFromFile(psName);
		psProg = psF.c_str();
	}
	if (!loadShaderCode(vsProg, psProg)) {
		vsMode = this->vsMode;
		psMode = this->psMode;

		if (vsMode == VS_DEFAULT) vsProg = shadowDiffuseVertexProgram;
		if (vsMode == VS_4BONES) vsProg = shadowDiffuseVertexProgram4Bones;
		if (vsMode == VS_8BONES) vsProg = shadowDiffuseVertexProgram8Bones;
		if (vsMode == VS_INSTANCE) vsProg = shadowDiffuseVertexProgramInstance;
		if (vsMode == VS_TEXINSTANCE) vsProg = shadowDiffuseVertexProgramTexInstance;


		if (psMode == PS_SHADE) psProg = shadowDiffuseFragmentProgram;
		if (psMode == PS_SHADE3D) psProg = fragmentShader3DComposite.c_str();
		if (psMode == PS_WHITE) psProg = whiteFragmentProgram;
		if (psMode == PS_NORMAL) psProg = normalFragmentProgram;
		loadShaderCode(vsProg, psProg);
	}
	
	/*
	char cCurrentPath[FILENAME_MAX];

	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
	{
		return true;
	}


	std::string ps = LoadStringFromFile("kuluPS.cpp");
	if (!loadShaderCode(shadowDiffuseVertexProgram4Bones, ps.c_str())) {
		loadShaderCode(shadowDiffuseVertexProgram4Bones, shadowDiffuseFragmentProgram);
	}*/
	return true;
}




bool ShaderShadow::setUniform(const char* name, const PxMat33& value) {
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform(name, value);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform(name, value);		
	}
	return Shader::setUniform(name, value);
}
bool ShaderShadow::setUniform(const char* name, const PxTransform& value) {
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform(name, value);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform(name, value);
		
	}
	return Shader::setUniform(name, value);
}
bool ShaderShadow::setUniform(const char *name, PxU32 size, const PxVec3* value) {
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform(name, size, value);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform(name, size, value);
		
	}
	return Shader::setUniform(name, size, value);
}

bool ShaderShadow::setUniform1(const char* name, float val) {
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform1(name, val);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform1(name, val);
		
	}
	return Shader::setUniform1(name, val);

}
bool ShaderShadow::setUniform2(const char* name, float val0, float val1){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform2(name, val0, val1);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform2(name, val0, val1);
		
	}
	return Shader::setUniform2(name, val0, val1);
}
bool ShaderShadow::setUniform3(const char* name, float val0, float val1, float val2){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform3(name, val0, val1, val2);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform3(name, val0, val1, val2);
		
	}
	return Shader::setUniform3(name, val0, val1, val2);
}
bool ShaderShadow::setUniform4(const char* name, float val0, float val1, float val2, float val3){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform4(name, val0, val1, val2, val3);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform4(name, val0, val1, val2, val3);
		
	}
	return Shader::setUniform4(name, val0, val1, val2, val3);
}
bool ShaderShadow::setUniformfv(const GLchar *name, GLfloat *v, int elementSize, int count){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniformfv(name, v, elementSize, count);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniformfv(name, v, elementSize, count);
		
	}
	return Shader::setUniformfv(name, v, elementSize, count);
}

bool ShaderShadow::setUniform(const char* name, float value){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform(name, value);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform(name, value);
		
	}
	return Shader::setUniform(name, value);
}
bool ShaderShadow::setUniform(const char* name, int value){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniform(name, value);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniform(name, value);
		
	}
	return Shader::setUniform(name, value);
}
bool ShaderShadow::setUniformMatrix4fv(const GLchar *name, const GLfloat *m, bool transpose){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setUniformMatrix4fv(name, m, transpose);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setUniformMatrix4fv(name, m, transpose);
		
	}
	return Shader::setUniformMatrix4fv(name, m, transpose);
}

GLint ShaderShadow::getUniformCommon(const char* name){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->getUniformCommon(name);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->getUniformCommon(name);
	}
	return Shader::getUniformCommon(name);
}

bool ShaderShadow::setAttribute(const char* name, PxU32 size, PxU32 stride, GLenum type, void* data){
	if ((renderMode == RENDER_DEPTH) && (psMode != PS_WHITE)) {
		return whiteShader->setAttribute(name, size, stride, type, data);
		
	}
	if ((renderMode == RENDER_DEPTH_NORMAL) && (psMode != PS_NORMAL)) {
		return normalShader->setAttribute(name, size, stride, type, data);
		
	}
	return Shader::setAttribute(name, size, stride, type, data);
}


