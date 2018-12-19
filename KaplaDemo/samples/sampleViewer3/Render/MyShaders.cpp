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

// GLSL shaders
#include "Compound.h"
#define STRINGIFY(A) #A

// particle vertex shader
const char *particleVS = STRINGIFY(
uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels
uniform float densityThreshold = 500.0;
uniform float pointShrink = 0.2;
void main()
{
    // scale down point size based on density
    float density = gl_MultiTexCoord1.x;
    float scaledPointRadius = pointRadius * (pointShrink + smoothstep(densityThreshold, densityThreshold*2.0, density)*(1.0-pointShrink));
//    float scaledPointRadius = pointRadius;

    // calculate window-space point size
    vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
    float dist = length(eyeSpacePos.xyz);
    gl_PointSize = scaledPointRadius * (pointScale / dist);

    gl_TexCoord[0] = gl_MultiTexCoord0;                         // sprite texcoord
    gl_TexCoord[1] = vec4(eyeSpacePos.xyz, scaledPointRadius);  // eye space pos
    gl_TexCoord[2] = vec4(gl_MultiTexCoord2.xyz, density);      // velocity and density
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    if (density < densityThreshold) gl_Position.w = -1.0;	// cull particles with small density

    gl_FrontColor = gl_Color;
}
);
const char *particleVSNoKill = STRINGIFY(
uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels
uniform float densityThreshold = 500.0;
uniform float pointShrink = 0.2;
void main()
{
    // scale down point size based on density
    float density = gl_MultiTexCoord1.x;
//    float scaledPointRadius = pointRadius * (pointShrink + smoothstep(densityThreshold, densityThreshold*2.0, density)*(1.0-pointShrink))*max(0.0f, min(gl_MultiTexCoord3.x, 0.1))*10.0f;;
    float scaledPointRadius = pointRadius;

    // calculate window-space point size
    vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
    float dist = length(eyeSpacePos.xyz);
    gl_PointSize = scaledPointRadius * (pointScale / dist);
	//gl_PointSize = 10.0f;
    gl_TexCoord[0] = gl_MultiTexCoord0;                         // sprite texcoord
    gl_TexCoord[1] = vec4(eyeSpacePos.xyz, scaledPointRadius);  // eye space pos
    gl_TexCoord[2] = vec4(gl_MultiTexCoord2.xyz, density);      // velocity and density
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    gl_FrontColor = gl_Color;
}
);
// render particle as constant shaded disc
const char *particleDebugPS = STRINGIFY(
uniform float pointRadius;
void main()
{
    // calculate eye-space sphere normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;   // kill pixels outside circle

//    gl_FragColor = gl_Color;
    gl_FragColor = gl_TexCoord[2].w * 0.001;  // show density
//    gl_FragColor = gl_TexCoord[2];  // show vel
}
);

// render particle as lit sphere
const char *particleSpherePS = STRINGIFY(
uniform float pointRadius;
uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
void main()
{
    // calculate eye-space sphere normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;   // kill pixels outside circle
    N.z = sqrt(1.0-r2);

    // calculate depth
    vec4 eyeSpacePos = vec4(gl_TexCoord[1].xyz + N*pointRadius, 1.0);   // position of this pixel on sphere in eye space
    vec4 clipSpacePos = gl_ProjectionMatrix * eyeSpacePos;
    gl_FragDepth = (clipSpacePos.z / clipSpacePos.w)*0.5+0.5;

    float diffuse = max(0.0, dot(N, lightDir));

    gl_FragColor = diffuse*gl_Color;
}
);

// visualize density
const char *particleDensityPS = STRINGIFY(
uniform float pointRadius;
uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform float densityScale;
uniform float densityOffset;
void main()
{
    // calculate eye-space sphere normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;   // kill pixels outside circle
    N.z = sqrt(1.0-r2);

    // calculate depth
    vec4 eyeSpacePos = vec4(gl_TexCoord[1].xyz + N*pointRadius, 1.0);   // position of this pixel on sphere in eye space
    vec4 clipSpacePos = gl_ProjectionMatrix * eyeSpacePos;
    gl_FragDepth = (clipSpacePos.z / clipSpacePos.w)*0.5+0.5;

    float diffuse = max(0.0, dot(N, lightDir));

    // calculate color based on density
    float x = 1.0 - saturate((gl_TexCoord[2].w  - densityOffset) * densityScale);
    vec3 color = lerp(gl_Color.xyz, vec3(1.0, 1.0, 1.0), x);

    gl_FragColor = vec4(diffuse*color, 1.0);
}
);

// renders eye-space depth
const char *particleSurfacePS = STRINGIFY(
uniform float pointRadius;
uniform float faceScale = 1.0;
void main()
{
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;   // kill pixels outside circle
    N.z = sqrt(1.0-r2)*faceScale;

    // calculate depth
    vec4 eyeSpacePos = vec4(gl_TexCoord[1].xyz + N*gl_TexCoord[1].w, 1.0); // position of this pixel on sphere in eye space
    vec4 clipSpacePos = gl_ProjectionMatrix * eyeSpacePos;
    gl_FragDepth = (clipSpacePos.z / clipSpacePos.w)*0.5+0.5;

    gl_FragColor = eyeSpacePos.z; // output eye-space depth
//    gl_FragColor = -eyeSpacePos.z/10.0;
}
);

// render particle thickness
const char *particleThicknessPS = STRINGIFY(
uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform float pointRadius;
uniform float faceScale = 1.0;
void main()
{
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0) discard;   // kill pixels outside circle
    N.z = sqrt(1.0-r2) * faceScale;

    // calculate depth
    vec4 eyeSpacePos = vec4(gl_TexCoord[1].xyz + N*gl_TexCoord[1].w, 1.0); // position of this pixel on sphere in eye space
    vec4 clipSpacePos = gl_ProjectionMatrix * eyeSpacePos;
    gl_FragDepth = (clipSpacePos.z / clipSpacePos.w)*0.5+0.5;

    float alpha = exp(-r2*2.0);

//    gl_FragColor = eyeSpacePos.z;     // output distance
    gl_FragColor = N.z*pointRadius*2.0*alpha; // output thickness
}
);

// motion blur shaders
const char *mblurVS = STRINGIFY(
uniform float timestep = 0.02;
uniform vec3 eyeVel;
uniform float iStartFade = 1.0;
void main()
{
	vec3 pos = gl_Vertex.xyz;
	vec3 vel = gl_MultiTexCoord2.xyz;
	//vel = vec3(10.0f,0.0f,0.0f);
	vec3 pos2 = (pos - (vel+eyeVel)*timestep);   // previous position

	gl_Position = gl_ModelViewMatrix * vec4(pos, 1.0);     // eye space
	gl_TexCoord[0] = gl_ModelViewMatrix * vec4(pos2, 1.0);
	gl_TexCoord[1].x = gl_MultiTexCoord1.x;
	gl_TexCoord[1].y = max(0.0f, min(gl_MultiTexCoord3.x*iStartFade, 1.0f));
	gl_TexCoord[2].xyz = pos;
	gl_TexCoord[3] = gl_MultiTexCoord4;

	gl_FrontColor = gl_Color;
}

);

const char *mblurGS = STRINGIFY(
//#version 120\n
//#extension GL_EXT_geometry_shader4 : enable\n
uniform float pointRadius;  // point size in world space
uniform float densityThreshold = 50.0;
uniform float idensityThreshold = 1.0 / 50.0;
uniform float pointShrink = 0.25;
void main()
{
    gl_FrontColor = gl_FrontColorIn[0];
    float density = gl_TexCoordIn[0][1].x;
	float life = gl_TexCoordIn[0][1].y;

    // scale down point size based on density
    float pointSize = pointRadius;

	pointSize *= gl_TexCoordIn[0][3].x;

    // eye space
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 pos2 = gl_TexCoordIn[0][0].xyz;
    vec3 motion = pos - pos2;
    vec3 dir = normalize(motion);
    float len = length(motion);

    vec3 x = dir * pointSize;
    vec3 view = normalize(-pos);
    vec3 y = normalize(cross(dir, view)) * pointSize;
    float facing = dot(view, dir);

    // check for very small motion to avoid jitter
    float threshold = 0.01;
//    if (len < threshold) {
    if ((len < threshold) || (facing > 0.95) || (facing < -0.95)) {
        pos2 = pos;
        x = vec3(pointSize, 0.0, 0.0);
        y = vec3(0.0, -pointSize, 0.0);
    }

	
    
    //if (density < densityThreshold) {

        gl_TexCoord[0] = vec4(0, 0, 0, life);
        gl_TexCoord[2] =  vec4(pos + x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();

        gl_TexCoord[0] = vec4(0, 1, 0, life);
        gl_TexCoord[2] = vec4(pos + x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 0, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 1, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();


    //}
}
);
const char *mblurGSNoKill = STRINGIFY(
//#version 120\n
//#extension GL_EXT_geometry_shader4 : enable\n
uniform float pointRadius;  // point size in world space
uniform float densityThreshold = 50.0;
uniform float idensityThreshold = 1.0 / 30.0;
uniform float pointShrink = 0.25;
uniform sampler2D meteorTex;
void main()
{
    gl_FrontColor = gl_FrontColorIn[0];
    float density = gl_TexCoordIn[0][1].x;
	float life = gl_TexCoordIn[0][1].y;
	

	gl_TexCoord[1].xy = 0.25f*vec2(gl_PrimitiveIDIn / 4, gl_PrimitiveIDIn % 4);
    // scale down point size based on density
	float factor = 1.0f;//density * idensityThreshold;
	//smoothstep(0.0f, densityThreshold, density);
		//density * idensityThreshold;
		//clamp(density / 50.0f, 0, 1);
    float pointSize = pointRadius*factor;//*(pointShrink + smoothstep(0.0, densityThreshold, density)*(1.0-pointShrink));

	pointSize *= gl_TexCoordIn[0][3].x;
	float tmp = gl_TexCoordIn[0][3].y;

	float bb = 1.0f;
	if (tmp > 0.5f) {
		//gl_FrontColor = vec4(3*life,0,0,1);
		// TODO: Meteor trail color here...
		//vec2 fetchPos = vec2( min(max((3-lifeTime)/3,0),1), 0);
		float val = 1-min(max((life-0.3)/0.2,0.01),0.99);
		vec2 fetchPos = vec2(val, 0);
		gl_FrontColor = texture2D(meteorTex, fetchPos);
		if (gl_FrontColor.r > 0.5) bb += (gl_FrontColor.r-0.5)*(gl_FrontColor.r-0.5)*10;
		
	}
//    float pointSize = pointRadius;

    // eye space
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 pos2 = gl_TexCoordIn[0][0].xyz;
    vec3 motion = pos - pos2;
    vec3 dir = normalize(motion);
    float len = length(motion);

    vec3 x = dir * pointSize;
    vec3 view = normalize(-pos);
    vec3 y = normalize(cross(dir, view)) * pointSize;
    float facing = dot(view, dir);

    // check for very small motion to avoid jitter
    float threshold = 0.01;
//    if (len < threshold) {
    if ((len < threshold) || (facing > 0.95) || (facing < -0.95)) {
        pos2 = pos;
        x = vec3(pointSize, 0.0, 0.0);
        y = vec3(0.0, -pointSize, 0.0);
    }

	float angle = density;
	float cv = cos(angle);
	float sv = sin(angle);

	vec3 xt = cv*x + sv*y;
	vec3 yt = -sv*x + cv*y;
	x = xt;
	y = yt;

    {

        gl_TexCoord[0] = vec4(0, 0, bb, life);
        gl_TexCoord[2] = vec4(pos + x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();

        gl_TexCoord[0] = vec4(0, 1, bb, life);
		gl_TexCoord[2] = vec4(pos + x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();

        gl_TexCoord[0] = vec4(1, 0, bb, life);
		gl_TexCoord[2] = vec4(pos2 - x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();

        gl_TexCoord[0] = vec4(1, 1, bb, life);
        gl_TexCoord[2] = vec4(pos2 - x - y, 1);
		gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();
/*
        gl_TexCoord[0] = vec4(0, 0, 0, life);
        gl_TexCoord[2] =  vec4(pos + x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(0, 1, 0, life);
        gl_TexCoord[2] = vec4(pos + x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 0, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 1, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();
		*/
    }
}
);

const char *particleSprayPS = STRINGIFY(
uniform sampler2DArrayShadow stex;
uniform float shadowAmbient = 0.5;
uniform vec3 lightDir;
float shadowCoef()
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
void main()
{
	//gl_FragColor = vec4(1.0f,1.0f,1.0f,1.0f);
	//return;
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle

    float falloff = exp(-mag*4.0);
	float shadowC = shadowCoef();

    gl_FragColor = gl_Color*(shadowAmbient + (1.0f -shadowAmbient)*shadowC)*(abs(dot(lightDir, N))*0.3f+0.7f );//*falloff;

	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(gl_TexCoord[0].z*gl_TexCoord[0].z));
	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragColor.w *= falloff * gl_TexCoord[0].w;

}
);
const char *particleSprayGenFOMPS= STRINGIFY(
const float PI = 3.1415926535897932384626433832795;
const vec4 factor_a = vec4(2.0*PI)*vec4(0.0,1.0,2.0,3.0);
const vec4 factor_b = vec4(2.0*PI)*vec4(1.0,2.0,3.0,0.0);
const vec4 factor_m2 = vec4(-2.0);
uniform float ispotMaxDist;
void main()
{
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle

   // float falloff = exp(-mag*4.0);
	//float falloff = pow(1.0-mag,1.5);//exp(-mag);
	float falloff = 1.0;
	float opacity = gl_Color.a * falloff * min(gl_TexCoord[0].w,1.0f);

	float distance = sqrt(dot(gl_TexCoord[2].xyz, gl_TexCoord[2].xyz))*ispotMaxDist;


	//compute value for projection into Fourier basis
	vec4 cos_a0123 = cos(factor_a*vec4(distance));
	vec4 sin_b123  = sin(factor_b*vec4(distance));
	vec4 lnOpacitR = factor_m2*vec4(log(1.0-opacity));

	gl_FragData[0]  = lnOpacitR*cos_a0123;
	gl_FragData[1]  = lnOpacitR*sin_b123;
}
);

const char *particleSprayUseFOMPS = STRINGIFY(
uniform sampler2DArrayShadow stex;
uniform float shadowAmbient = 0.5;
float shadowCoef()
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
uniform float ispotMaxDist;
uniform vec3 spotOriginEye;
uniform sampler2D spot_a0123;
uniform sampler2D spot_b123;

const float PI = 3.1415926535897932384626433832795;
const vec3 _2pik = vec3(2.0) * vec3(PI,2.0*PI,3.0*PI);
const vec3 factor_a = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 factor_b = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 value_1 = vec3(1.0);

uniform mat4 eyeToSpotMatrix;
void main()
{
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle

    float falloff = pow(1.0-mag,1.0);//exp(-mag);
	//falloff = 1.0f;
	float shadowC = 1.0f;//shadowCoef();

	// Also FOM
	
//	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	//gl_FragColor.xyz = gl_TexCoord[3].xyz*0.25f;
	//gl_FragColor.xyz = projectionCoordinate.xyz / projectionCoordinate.w;
	//gl_FragColor.w = 1.0f;
	
	//read Fourier series coefficients for color extinction on RGB
	vec4 sR_a0123 = texture2DProj(spot_a0123,projectionCoordinate);
	vec3 sR_b123 = texture2DProj(spot_b123,projectionCoordinate).rgb;

	//gl_FragColor.xyz = sR_a0123.xyz;
	//gl_FragColor.w = 1.0f;
	//return;
	//compute absolute and normalized distance (in spot depth range)
	float distance2spotCenter = length(spotOriginEye-gl_TexCoord[2].xyz);//distance from spot origin to surfel in world space
	float d = distance2spotCenter*ispotMaxDist;

	
	//compute some value to recover the extinction coefficient using the Fourier series
	vec3 sin_a123    = sin(factor_a*vec3(d));
	vec3 cos_b123    = value_1-cos(factor_b*vec3(d));

	//compute the extinction coefficients using Fourier
	float att = (sR_a0123.r*d/2.0) + dot(sin_a123*(sR_a0123.gba/_2pik) ,value_1) + dot(cos_b123*(sR_b123.rgb/_2pik) ,value_1);

	att = max(0.0f, att);
	att = min(1.0f, att);
	shadowC = (1.0f-att);
	
	//....

    gl_FragColor.xyz = gl_Color.xyz*(shadowAmbient + (1.0f -shadowAmbient)*shadowC);//*falloff;
	gl_FragColor.w = gl_Color.w;

	//float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(gl_TexCoord[0].z*gl_TexCoord[0].z));
	//gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragColor.w *= max(min(falloff,1.0f),0.0f) * max(min(gl_TexCoord[0].w,1.0f),0.0f);

//	gl_FragColor.w = 0.2f;
	//gl_FragColor.w = falloff * gl_TexCoord[0].w;
	//gl_FragColor.xyz = sR_a0123.xyz;
}
);

// motion blur shaders
const char *particleDumbFoamVS = STRINGIFY(
uniform float timestep = 0.02;
// GPU Water
uniform sampler2D waterHFTex;
uniform float minEta;
uniform float fracEta;
uniform vec2 hfScale; // 1/(sx*dx), 1/(sz*dx)
uniform vec2 hfOffset; // 0.5/sx, 0.5/sz
uniform vec2 hfCellOffset; // 1.0/sx, 1.0/sz 
uniform vec2 origin;
uniform float dx;
uniform float waterHOffset;
uniform vec4 foamFactor = vec4(1.0/4, 1.0/4,0,0); 
void main()
{
	vec2 hfTexCoord = (gl_Vertex.xz - origin)*hfScale + hfOffset;
	vec4 etaH = texture2D(waterHFTex, hfTexCoord);	
	vec4 etaHxp1 = texture2D(waterHFTex, hfTexCoord + hfCellOffset*vec2(1,0));
	vec4 etaHxm1 = texture2D(waterHFTex, hfTexCoord + hfCellOffset*vec2(-1,0));
	vec4 etaHzp1 = texture2D(waterHFTex, hfTexCoord + hfCellOffset*vec2(0,1));
	vec4 etaHzm1 = texture2D(waterHFTex, hfTexCoord + hfCellOffset*vec2(0,-1));
	float h = etaH.x+etaH.y;
	float hxp1 = etaHxp1.x+etaHxp1.y;
	float hxm1 = etaHxm1.x+etaHxm1.y;
	float hzp1 = etaHzp1.x+etaHzp1.y;
	float hzm1 = etaHzm1.x+etaHzm1.y;
	
    vec3 pos = gl_Vertex.xyz;
    vec3 vel = gl_MultiTexCoord2.xyz;

	vec3 normal = vec3(-normalize(cross(vec3(2*dx, hxp1-hxm1, 0), vec3(0, hzp1-hzm1, 2*dx))));//vec3(0,1,0);
	vec3 bitangent = normalize(cross(vec3(1,0,0), normal));
	vec3 tangent = normalize(cross(bitangent, normal));

    gl_Position = gl_ModelViewMatrix * vec4(pos, 1.0);     // eye space
	gl_TexCoord[0] = gl_MultiTexCoord2 * foamFactor;
    gl_TexCoord[1] = gl_ModelViewMatrix * vec4(tangent, 0.0);
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(normal, 0.0);
	gl_TexCoord[3] = gl_ModelViewMatrix * vec4(bitangent, 0.0);

	gl_TexCoord[0].w = max(0.0f, min(gl_MultiTexCoord3.x, 0.1))*10;// Fade with life time
    gl_FrontColor = gl_Color;
}
);
const char *particleFoamVS = STRINGIFY(
uniform float timestep = 0.02;
// GPU Water
uniform float minEta;
uniform float fracEta;

uniform float dx;
uniform vec4 foamFactor = vec4(1.0/4, 1.0/4,0,0); 
uniform float foamOffset;
uniform float foamOffset2;
uniform float foamOffset3;
void main()
{

	
    vec3 pos = gl_Vertex.xyz;

	vec3 normal = normalize(gl_MultiTexCoord2.xyz);
	vec3 bitangent = normalize(cross(vec3(1,0,0), normal));
	vec3 tangent = normalize(cross(bitangent, normal));

    gl_Position = gl_ModelViewMatrix * vec4(pos + normal*foamOffset + vec3(0.0f, foamOffset3, 0.0f), 1.0);     // eye space
	vec3 eyeVec = normalize(gl_Position.xyz);
	gl_Position.xyz -= eyeVec*foamOffset2;
	gl_TexCoord[0] = gl_MultiTexCoord2 * foamFactor;
    gl_TexCoord[1] = gl_ModelViewMatrix * vec4(tangent, 0.0);
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(normal, 0.0);
	gl_TexCoord[3] = gl_ModelViewMatrix * vec4(bitangent, 0.0);

	gl_TexCoord[0].w = max(0.0f, min(gl_MultiTexCoord3.x, 0.1))*10;// Fade with life time
    gl_FrontColor = gl_Color;
}
);

const char *particleDumbFoamGS = STRINGIFY(
//#version 120\n
//#extension GL_EXT_geometry_shader4 : enable\n
uniform float pointRadius;  // point size in world space
uniform float densityThreshold = 500.0;
uniform float pointShrink = 0.25;
uniform float foamTexSize = 0.249;
void main()
{
    gl_FrontColor = gl_FrontColorIn[0];
    // eye space
    vec3 pos = gl_PositionIn[0].xyz;
	vec3 x = pointRadius*gl_TexCoordIn[0][1].xyz;
	vec3 normal = gl_TexCoordIn[0][2].xyz;
	vec3 y = -pointRadius*gl_TexCoordIn[0][3].xyz;

    

    gl_TexCoord[0] = gl_TexCoordIn[0][0]+vec4(0, 0, 0, 0);
	gl_TexCoord[1] = vec4(normal, gl_TexCoordIn[0][0].w);
    gl_TexCoord[2] =  vec4(pos + x + y, 1);
	gl_TexCoord[3] = vec4(1, 1, 0, 0);
    gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
    EmitVertex();

    gl_TexCoord[0] = gl_TexCoordIn[0][0]+vec4(0, foamTexSize, 0, 0);
    gl_TexCoord[2] = vec4(pos + x - y, 1);
	gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
	gl_TexCoord[3] = vec4(1, -1, 0, 0);
    EmitVertex();

    gl_TexCoord[0] = gl_TexCoordIn[0][0]+vec4(foamTexSize, 0, 0, 0);
     gl_TexCoord[2] = vec4(pos - x + y, 1);
	gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
	gl_TexCoord[3] = vec4(-1, 1, 0, 0);
    EmitVertex();

    gl_TexCoord[0] = gl_TexCoordIn[0][0]+vec4(foamTexSize, foamTexSize, 0, 0);
     gl_TexCoord[2] = vec4(pos - x - y, 1);
	gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
	gl_TexCoord[3] = vec4(-1, -1, 0, 0);
    EmitVertex();

}
);

const char *particleDumbFoamPS = STRINGIFY(
uniform float pointRadius; 
uniform vec3 lightDir;
uniform sampler2D foamTex;
uniform sampler2DArrayShadow stex;
float shadowCoef()
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
uniform float foamAmbient = 0.8f;
uniform float foamDiffuse = 0.2f;
void main()
{
     // calculate eye-space normal from texture coordinates

	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	float mag = clamp(1.1-dot(gl_TexCoord[3].xy,gl_TexCoord[3].xy),0,1);


	gl_FragColor.xyz = mix(gl_Fog.color, gl_Color*(foamAmbient + (foamDiffuse*max(dot(gl_TexCoord[1].xyz, lightDir),0.0f) )*shadowCoef()), fog);//*falloff;
	gl_FragColor.w = /*texture2D(foamTex, gl_TexCoord[0]).r**/gl_Color.w*mag*gl_TexCoord[1].w;
//

}
);

const char *particleFoamPS = STRINGIFY(
uniform float pointRadius; 
uniform vec3 lightDir;
uniform sampler2D foamTex;
uniform sampler2DArrayShadow stex;
float shadowCoef()
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
uniform float foamAmbient = 0.5f;
uniform float foamDiffuse = 0.5f;
void main()
{
     // calculate eye-space normal from texture coordinates

	//float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	float mag = clamp(1.1-dot(gl_TexCoord[3].xy,gl_TexCoord[3].xy),0,1);


	gl_FragColor.xyz = gl_Color*(
						foamAmbient + (foamDiffuse*abs(dot(gl_TexCoord[1].xyz, lightDir)))*shadowCoef()
						);//*falloff;
	gl_FragColor.w = /*texture2D(foamTex, gl_TexCoord[0]).r**/gl_Color.w*mag*gl_TexCoord[1].w;
//

}
);

// screen-space shaders
const char *passThruVS = STRINGIFY(
void main()
{
    gl_Position = gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_Vertex;
    gl_FrontColor = gl_Color;
}
);

// blur depth map
// separable version
const char *depthBlurPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
sampler2DRect colorTex;
uniform vec2 scale = 1.0;
const float r = 10.0;
uniform float blurScale = 2.0 / r;
uniform float blurDepthFalloff = 2.0;
uniform float depthThreshold = 0.5;
void main()
{
    float center = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (center < -9999.0) {
        // skip background pixels
        discard;
        return;
    }

    float sum = 0;
    float wsum = 0;
    for(float x=-r; x<=r; x+=1.0) {
        float sample = texture2DRect(colorTex, gl_TexCoord[0].xy + x*scale).x;

        // bilateral filter
        // spatial domain
        float r = x * blurScale;
        float w = exp(-r*r);
        //float w = 1.0;

        // range domain (based on depth difference)
        float r2 = (sample - center) * blurDepthFalloff;
        //float g = 1.0;
        float g = exp(-r2*r2);
        //float g = abs(sample - center) < depthThreshold;

        sum += sample * w * g;
        wsum += w * g;
    }

    if (wsum > 0.0) {
        sum /= wsum;
    }
    gl_FragColor.x = sum;
}
);

// 2D non-separable version
const char *depthBlur2DPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
sampler2DRect colorTex;
uniform vec2 scale = 1.0;
const float r = 10.0;
uniform float blurScale = 2.0 / r;
uniform float blurDepthFalloff = 1.0;
uniform float depthThreshold = 0.1;
void main()
{
    float center = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (center < -9999.0) {
        discard;
        return;
    }

    float sum = 0;
    float wsum = 0;
    for(float y=-r; y<=r; y+=1.0) {
        for(float x=-r; x<=r; x+=1.0) {
            float sample = texture2DRect(colorTex, gl_TexCoord[0].xy + vec2(x, y)).x;

            // bilateral filter
            // spatial domain
			//float r = length(vec2(x, y)) * blurScale;
   //         float w = exp(-r*r);
            float rsq = (x*x + y*y) * blurScale * blurScale;
            float w = exp(-rsq);
            //float w = 1.0;

            // range domain (based on depth difference)
            float r2 = (sample - center) * blurDepthFalloff;
            //float g = 1.0;
            float g = exp(-r2*r2);
//            float g = abs(sample - center) < depthThreshold;

            sum += sample * w * g;
            wsum += w * g;
        }
    }

    if (wsum > 0.0) {
        sum /= wsum;
    }
    gl_FragColor.x = sum;
}
);

// symmetrical filter (see "Screen Space Meshes" paper)
const char *depthBlurSymPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
sampler2DRect colorTex;
uniform vec2 scale = 1.0;
const float r = 10.0;
uniform float blurScale = 2.0;
uniform float blurDepthFalloff = 2.0;
uniform float depthThreshold = 0.25;
void main()
{
    float center = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (center == 0.0) {
        discard; return;
    }

    float sum = center;
    float wsum = 1.0;

    for(float x=1.0; x<=r; x+=1.0) {
        float sample = texture2DRect(colorTex, gl_TexCoord[0].xy + x*scale).x;
        float sample2 = texture2DRect(colorTex, gl_TexCoord[0].xy - x*scale).x;

        bool valid = abs(sample - center) < depthThreshold;
        bool valid2 = abs(sample2 - center) < depthThreshold;

        if (valid && valid2) {
            float r = (x / r) * blurScale;
            float w = exp(-r*r);

            sum += sample * w;
            wsum += w;

            sum += sample2 * w;
            wsum += w;
        }
    }

    if (wsum > 0.0) {
        sum /= wsum;
    }
    gl_FragColor.x = sum;
}
);

// display final shaded surface
const char *displaySurfacePS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform sampler2DRect thicknessTex;
uniform sampler2DRect sceneTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.1;
uniform float shininess = 40.0;
uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(1.0, 1.0, 1.0, 1.0); 
//uniform vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0); 

uniform float falloffScale = 0.3;
//uniform float falloffScale = 0.1;
uniform vec3 thicknessRefraction = vec3(2.0, 2.3, 2.6);
uniform float subSample = 1;

uniform float fresnelBias = 0.1;
uniform float fresnelScale = 0.4;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct

// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
    uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
    return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
    float eyeZ = texture2DRect(tex, texCoord).x;
    return uvToEye(texCoord*invViewport, eyeZ);
}

void main()
{
    float c = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (c < -9999.0) {
        discard;
        return;
    }

    // calculate normal
    // taking silohette edges into account
    vec2 uv = gl_TexCoord[0].xy * invViewport;
    vec3 eyePos = uvToEye(uv, c);

    vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
    if (abs(ddx.z) > depthThreshold) {
        ddx = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
    }

    vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
    if (abs(ddy.z) > depthThreshold) { 
        ddy = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
    }

    vec3 n = cross(ddx, ddy);
    n = normalize(n);

    // lighting
//    float diffuse = max(0.0, dot(n, lightDir));
    float diffuse = dot(n, lightDir)*0.5+0.5; // wrap lighting

    vec3 v = normalize(-eyePos);
    vec3 h = normalize(lightDir + v);
    float specular = pow(max(0.0, dot(n, h)), shininess);

    float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

    // cubemap
    vec3 r = reflect(-v, n);
    r = r * gl_NormalMatrix;
    vec4 reflectColor = textureCube(cubemapTex, r);

    // color attenuation based on thickness
    float thickness = texture2DRect(thicknessTex, gl_TexCoord[0].xy * subSample).x;
    vec4 attenuatedColor = fluidColor * exp(-thickness*falloffScale*colorFalloff);
//    vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);

    // refraction
    float refraction = thickness*thicknessRefraction.x;
    vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
/*
    // dispersive refraction
    vec3 refraction = thickness*thicknessRefraction;
    vec4 sceneCol;
    sceneCol.r = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.x)).r;
    sceneCol.g = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.y)).g;
    sceneCol.b = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.z)).b;
    sceneCol.a = 1.0;
*/

    vec4 finalCol = vec4(attenuatedColor.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);
    float alpha = saturate(attenuatedColor.w);

//    gl_FragColor = -c.z;
//    gl_FragColor = vec4(n*0.5+0.5, 1.0);
//	  gl_FragColor = vec4(diffuse.xxx, 1.0);
//	  gl_FragColor = vec4(specular.xxx, 1.0);
//	  gl_FragColor = vec4(fresnel.xxx, 1.0);
//    gl_FragColor = diffuse*fluidColor + specular;
//    gl_FragColor = vec4(eyePos*0.5+0.5, 1.0);
//    gl_FragColor = thickness;
//    gl_FragColor = vec4(attenuatedColor.xyz, 1.0);
//    gl_FragColor = attenuatedColor.w;
//    gl_FragColor = alpha;

    gl_FragColor = lerp(finalCol, sceneCol, alpha);
	
//    gl_FragColor = sceneCol*attenuatedColor + reflectColor*fresnel + specularColor*specular;

//    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * subSample).x;
	//gl_FragColor.w = 1.0;
}
);

// new version (handles transparency differently)
const char *displaySurfaceNewPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform sampler2DRect thicknessTex;
uniform sampler2DRect sceneTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.1;
uniform float shininess = 40.0;
uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(0.5, 0.5, 0.5, 1.0); 

uniform float falloffScale = 0.01;
uniform vec3 thicknessRefraction = vec3(2.0, 2.1, 2.2);
uniform float subSample = 1;

uniform float fresnelBias = 0.1;
uniform float fresnelScale = 0.4;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct

// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
    uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
    return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
    float eyeZ = texture2DRect(tex, texCoord).x;
    return uvToEye(texCoord*invViewport, eyeZ);
}

void main()
{
    float c = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (c < -9999.0) {
        discard;
        return;
    }

    // calculate normal
    // taking silohette edges into account
    vec2 uv = gl_TexCoord[0].xy * invViewport;
    vec3 eyePos = uvToEye(uv, c);

    vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
    if (abs(ddx.z) > depthThreshold) {
        ddx = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
    }

    vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
    if (abs(ddy.z) > depthThreshold) { 
        ddy = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
    }

    vec3 n = cross(ddx, ddy);
    n = normalize(n);

    // lighting
//    float diffuse = max(0.0, dot(n, lightDir));
    float diffuse = dot(n, lightDir)*0.5+0.5; // wrap lighting

    vec3 v = normalize(-eyePos);
    vec3 h = normalize(lightDir + v);
    float specular = pow(max(0.0, dot(n, h)), shininess);

    float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

    // cubemap
    vec3 r = reflect(-v, n);
    r = r * gl_NormalMatrix;
    vec4 reflectColor = textureCube(cubemapTex, r);

    // color attenuation based on thickness
    float thickness = texture2DRect(thicknessTex, gl_TexCoord[0].xy * subSample).x;
    vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);

    // refraction
//    float refraction = thickness*thicknessRefraction.x;
//    vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));

    // dispersive refraction
    vec3 refraction = thickness*thicknessRefraction;
    vec4 sceneCol;
    sceneCol.r = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.x)).r;
    sceneCol.g = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.y)).g;
    sceneCol.b = texture2DRect(sceneTex, gl_TexCoord[0].xy + (n.xy * refraction.z)).b;
    sceneCol.a = 1.0;
    
    gl_FragColor = sceneCol*attenuatedColor + reflectColor*fresnel + specularColor*specular;

    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * subSample).x;
}
);

// non-transparent version for oil etc.
const char *displaySurfaceSolidPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.1;
uniform float shininess = 40.0;
uniform vec4 diffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
uniform vec4 specularColor = vec4(0.75, 0.75, 0.75, 1.0); 

uniform float subSample = 1;

uniform float fresnelBias = 0.1;
uniform float fresnelScale = 0.4;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct

// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
    uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
    return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
    float eyeZ = texture2DRect(tex, texCoord).x;
    return uvToEye(texCoord*invViewport, eyeZ);
}

void main()
{
    float c = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (c < -9999.0) {
        discard;
        return;
    }

    // calculate normal
    // taking silohette edges into account
    vec2 uv = gl_TexCoord[0].xy * invViewport;
    vec3 eyePos = uvToEye(uv, c);

    vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
    if (abs(ddx.z) > depthThreshold) {
        ddx = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
    }

    vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
    if (abs(ddy.z) > depthThreshold) { 
        ddy = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
    }

    vec3 n = cross(ddx, ddy);
    n = normalize(n);

    // lighting
    float diffuse = max(0.0, dot(n, lightDir));
//    float diffuse = dot(n, lightDir)*0.5+0.5; // wrap lighting

    vec3 v = normalize(-eyePos);
    vec3 h = normalize(lightDir + v);
    float specular = pow(max(0.0, dot(n, h)), shininess);

    float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

    // cubemap
    vec3 r = reflect(-v, n);
    r = r * gl_NormalMatrix;
    vec4 reflectColor = textureCube(cubemapTex, r);
    
    gl_FragColor = diffuseColor*diffuse + reflectColor*fresnel + specularColor*specular;

    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * subSample).x;
}
);

// chrome surface using cubemap
const char *displaySurfaceChromePS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.1;
uniform float shininess = 40.0;
uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);
uniform float subSample = 1;

// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
    uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
    return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
    float eyeZ = texture2DRect(tex, texCoord).x;
    return uvToEye(texCoord*invViewport, eyeZ);
}

void main()
{
    float c = texture2DRect(colorTex, gl_TexCoord[0].xy).x;
    if (c < -9999.0) {
        discard;
        return;
    }

    // calculate normal
    // taking silohette edges into account
    vec2 uv = gl_TexCoord[0].xy * invViewport;
    vec3 eyePos = uvToEye(uv, c);

    vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
    vec3 ddx2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
//    if (abs(ddx.z) > abs(ddx2.z)) {
    if (abs(ddx.z) > depthThreshold) { 
        ddx = ddx2;
    }

    vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
    vec3 ddy2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
//    if (abs(ddy2.z) < abs(ddy.z)) {
    if (abs(ddy.z) > depthThreshold) { 
        ddy = ddy2;
    }

    vec3 n = cross(ddx, ddy);
    n = normalize(n);

    // lighting
    float diffuse = max(0.0, dot(n, lightDir));
//    float diffuse = dot(n, lightDir)*0.5+0.5; // wrap lighting

//    vec3 v = vec3(0, 0, 1);
    vec3 v = normalize(-eyePos);
    vec3 h = normalize(lightDir + v);
    float specular = pow(max(0.0, dot(n, h)), shininess);

//    float fresnel = pow(1.0 - max(0.0, dot(n, v)), 5.0);
    float fresnel = 0.2 + 0.8*pow(1.0 - max(0.0, dot(n, v)), 2.0);
	//float fresnel = 1.0 - max(0.0, dot(n, v));

    // cubemap
    vec3 r = reflect(-v, n);
//    r = (mat3) gl_ModelViewMatrixInverse * r;
    r = r * gl_NormalMatrix;
    vec4 reflectColor = textureCube(cubemapTex, r);

//    gl_FragColor = reflectColor * fresnel;
    gl_FragColor = reflectColor * (0.5 + 0.5*diffuse);
    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * subSample).x;
}
);


const char *textureRectPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect tex;

void main()
{
    gl_FragColor = texture2DRect(tex, gl_TexCoord[0].xy);
}
);

// dilate depth image by taking maximum value of neighbourhood
const char *dilatePS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect tex;

void main()
{
    float c = texture2DRect(tex, gl_TexCoord[0].xy).x;
    if (c < -9999.0) {
        c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(1, 0)).x);
        c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(1, 1)).x);
        c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(0, 1)).x);

        //c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(-1, 0)).x);
        //c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(1, 0)).x);
        //c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(0, -1)).x);
        //c = max(c, texture2DRect(tex, gl_TexCoord[0].xy + vec2(0, 1)).x);
    }

    gl_FragColor = c;
}
);

// NUTT
// Water heightfield thickness vertex shader
const char *hfThicknessVS = STRINGIFY(
uniform float zMax;
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * vec4(gl_Vertex.xyz,1);
	

	vec4 eyeNormal = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0);
	//gl_TexCoord[0] = vec4(eyeSpacePos.xyz,0);                         // sprite texcoord
	
	//float tc = 0.1*(zFar + eyeSpacePos.z);
	float dis = sqrt(dot(eyeSpacePos,eyeSpacePos));
	float tc = (zMax-dis);
	//gl_FrontColor = vec4(tc,tc,-tc,1);
	//gl_BackColor = vec4(-tc,-tc,-tc,1);
	//float tc = 0.1;
	
	if (dot(eyeNormal.xyz,eyeSpacePos.xyz) > 0) {
		gl_TexCoord[0] = vec4(tc,tc,tc,1);		
	} else 
	{
		gl_TexCoord[0]= vec4(-tc,-tc,-tc,1);
		
	}
	gl_Position = gl_ProjectionMatrix * eyeSpacePos;
	//gl_BackColor = vec4(1,1,1,1);//vec4(-1000,-1000,-1000,1);//vec4(-2*tc,-2*tc,-2*tc,1);
}
);


// Water heightfield thickness PS
const char *hfThicknessPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
void main()
{
	
	gl_FragColor = gl_TexCoord[0];
	//gl_FragColor = gl_Color;
	//gl_FragColor = (
}
);

// Water heightfield thickness PS
const char *hfThicknessAddPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect thicknessTex;
void main()
{
	// Add contribution of heightfield fluid thickness to particle thickness
	vec4 col = texture2DRect(thicknessTex, gl_TexCoord[0].xy);
	col.x = max(col.x, 0.0);
	//col.x*=-1;
	/*
	if (col.x > 10) {
		gl_FragColor = vec4(0,0,0,1);
	} else { */
		gl_FragColor = vec4(col.x,col.x,col.x,1);
	//}
}
);
// Water heightfield depth vertex shader
const char *hfDepthVS = STRINGIFY(
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = vec4(eyeSpacePos.xyz,0);                         // sprite texcoord

}
);
// Water heightfield depth PS
const char *hfDepthPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
									  void main()
{	
	gl_FragColor = vec4(gl_TexCoord[0].z,gl_TexCoord[0].z,gl_TexCoord[0].z,1);
}
);

// Turn depth map to distance from eye and subtract
const char *depthToInitThicknessPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect depthTex;
uniform float zNear;
uniform float zFar;
uniform float mulX;
uniform float mulY;
uniform float zMax;

void main()
{
	float glDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
	float eyeDepth = zNear*zFar / (zFar - glDepth*(zFar-zNear));
	gl_FragDepth = glDepth;
	//gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
	//gl_FragColor = vec4(gl_FragDepth,gl_FragDepth,gl_FragDepth,1);
	//float val = 0.001*(-(zFar - eyeDepth));
	//float val = -(zFar-eyeDepth)*0.1;
	vec3 eyePos = vec3(gl_TexCoord[1].x*eyeDepth*mulX, gl_TexCoord[1].y*eyeDepth*mulY, eyeDepth);

	float val = -(zMax-sqrt(dot(eyePos,eyePos)));
	gl_FragColor = vec4( val,val,val, 1);

	
}
);

// Debug PS
const char *debugPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect thicknessTex;

void main()
{	
	vec4 col = texture2DRect(thicknessTex, gl_TexCoord[0].xy);

	col.x*=-0.01;
//	if (col.x > 1) {
//		gl_FragColor = vec4(1,0,0,1);
//	} else { 
		gl_FragColor = vec4(col.x,col.x,col.x,1);
//	}
	//gl_FragColor = vec4(1,0,1,1);
}
);


// Debug Triangle VS
const char *debugTriVS = STRINGIFY(								   
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	vec4 eyeSpaceNormal = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,1);
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec3 eyeVec = normalize(eyeSpacePos.xyz);
	float dp = dot(eyeSpaceNormal.xyz,eyeVec.xyz);

	if (dp > 0) {
		gl_FrontColor = vec4(dp,0,0,1);
	} else {
		gl_FrontColor = vec4(0,-dp,0,1);
	}
}
);
const char *copyPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform sampler2DRect depthTex;
uniform sampler2DRect sceneTex;
void main()
{
    gl_FragColor = texture2DRect(sceneTex, gl_TexCoord[0].xy);
	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
}
);


// Debug Triangle PS
const char *debugTriPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
void main()
{	
	gl_FragColor = gl_Color;
}
);

// display final shaded surface Nuttapong's mod
const char *displaySurfaceNutPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform vec2 colorTexScale;
uniform sampler2DArrayShadow stex;
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform sampler2DRect thicknessTex;
uniform sampler2DRect sceneTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.01;
uniform float shininess = 40.0;
//uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);
uniform vec4 fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(0.7, 0.7, 0.7, 1.0); 
//uniform vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0); 

uniform float falloffScale = 0.03;
//uniform float falloffScale = 0.1;
uniform vec3 thicknessRefraction = vec3(2.0, 2.3, 2.6);
uniform float subSample = 1;

uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct
uniform float shadowAmbient = 0.9;
uniform float splashZShadowBias = 0.001f;
uniform float refracMultiplier = 1.0;

uniform float epsilon = 0.0001f;
uniform float thicknessAlphaMul = 3.0f;
uniform float decayRate = 1.0f;
// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
	uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
	return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
	float eyeZ = texture2DRect(tex, texCoord*colorTexScale).x;
	return uvToEye(texCoord*invViewport, eyeZ);
}
float shadowCoef(float3 eyePos)
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(eyePos.x, eyePos.y, eyePos.z,1);
	
	shadow_coord.w = shadow_coord.z + splashZShadowBias;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}

void main()
{
	//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	//return;
	float c = texture2DRect(colorTex, gl_TexCoord[0].xy*colorTexScale).x;
	if (c < -9999.0) {
		discard;
		return;
	}

	// calculate normal
	// taking silohette edges into account
	vec2 uv = gl_TexCoord[0].xy * invViewport;
	vec3 eyePos = uvToEye(uv, c);

	
	vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
	vec3 ddx2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	

	vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
	vec3 ddy2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));

	 float thickness = texture2DRect(thicknessTex, gl_TexCoord[0].xy * subSample).x;
	 float refBlend = 0.0f;
	 float maxDz = max(max(abs(ddx.z),abs(ddx2.z)), max(abs(ddy.z),abs(ddy2.z)));
	 if (maxDz > depthThreshold) {
		refBlend = 1.0f - exp((depthThreshold - maxDz)*decayRate);
	 }

//    if (abs(ddx.z) > abs(ddx2.z)) {
	//if (abs(ddx.z) > depthThreshold) { 
		if (abs(ddx2.z) < abs(ddx.z)) {
			ddx = ddx2;
		}
	//}
	//    if (abs(ddy2.z) < abs(ddy.z)) {
	//if (abs(ddy.z) > depthThreshold) { 
		if (abs(ddy2.z) < abs(ddy.z)) {
			ddy = ddy2;
		}	
	//}


	ddx.x += epsilon;
	ddy.y += epsilon;
	

	//vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	//vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);

	// color attenuation based on thickness


	//ddx = normalize(ddx);
	//ddy = normalize(ddy);
	//if (depthThreshold < 0.001f) {
	//	vec3 nn = cross(ddx, ddy);
		//gl_FragColor = vec4(sqrt(dot(ddx,ddx)), sqrt(dot(ddy, ddy)), sqrt(dot(nn,nn)), 1.0f);
	//	gl_FragColor = vec4(10.0f*sqrt(dot(nn,nn)), 0.0f, 0.0f, 1.0f);
	//	return;
	//}
	
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);
	
	vec3 n = normalize(cross(ddx, ddy));


	// lighting
	//    float diffuse = max(0.0, dot(n, lightDir));
	float diffuse = dot(n, lightDir)*0.5+0.5; // wrap lighting

	vec3 v = normalize(-eyePos);
	vec3 h = normalize(lightDir + v);
	float specular = pow(max(0.0, dot(n, h)), shininess*10);

	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

	// cubemap
	vec3 r = reflect(-v, n);
	r = r * gl_NormalMatrix;
	vec4 reflectColor = textureCube(cubemapTex, r);

	
	//    vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);

	// refraction
	// -----------------
	// Pond
	//vec4 attenuatedColor = fluidColor * exp(-thickness*falloffScale*colorFalloff);
	//float refracMultiplier = 1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// Whirlpool
	//float refracMultiplier = 0.1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//sceneCol.xyz = lerp(vec3(0.13, 0.19, 0.22), sceneCol.xyz, blendFactor);

	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// FERMI
	float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));

	reflectColor = (1.0f-refBlend)*reflectColor + refBlend*sceneCol;

//	colorFalloff = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff*10.0);
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	//blendFactor = min(0.5, blendFactor);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//fluidColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//fluidColor = vec4(28/255.0, 69/255.0, 89/255.0, 0.0);
	//fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);
	sceneCol.xyz = lerp(fluidColor, sceneCol.xyz, attenuatedColor);
	//sceneCol = fluidColor;
	//sceneCol.xyz = vec3(0.0f,0.0f,0.0f);


	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel /*+ specularColor.xyz*specular*/, 1.0);
	float shadow = shadowCoef(eyePos);	
	float sc = (1.0f - shadowAmbient)*shadow + shadowAmbient;
	vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular*shadow*0.9, 1.0);

	gl_FragColor = vec4(finalCol.xyz*(sc), max(0.0f, min(1.0, thicknessAlphaMul*thickness/*2.0f*(gl_TexCoord[1].w+waterHOffset)*/)));

//	gl_FragColor = finalCol;
//	gl_FragColor.w = max(0.0f, min(1.0, 3.0f*thickness));

	/*
	vec4 sceneSeethroughCol = texture2DRect(sceneTex, gl_TexCoord[0].xy);


	//----------------
	float thickPar = 0.0000001;
	float blendBackground = max(0.0, min(1.0f, -thickness / thickPar));


	gl_FragColor = (blendBackground)*finalCol + (1.0-blendBackground)*sceneSeethroughCol;
	if (thickness < 0) gl_FragColor = vec4(1.0f,0.0f,0.0f,1.0f); else */
	//gl_FragColor = finalCol;
	//    gl_FragColor = sceneCol*attenuatedColor + reflectColor*fresnel + specularColor*specular;

	//    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
	float fog = clamp(gl_Fog.scale*(gl_Fog.end+eyePos.z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(eyePos.z*eyePos.z));

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * colorTexScale).x;
		//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//return;

	//gl_FragColor.w = max(0.0, min((thickness)*5.0, 1.0));
	//gl_FragColor.w = 1.0;
	
	//gl_FragColor = vec4(n.z, n.z, n.z, 1);
}
);



// render particle as lit sphere
const char *particleBubblePS = STRINGIFY(
									   uniform float pointRadius;
uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform samplerCube cubemapTex;
uniform sampler2DRect sceneTex;

uniform vec2 viewportd2 = vec2(300,300);
uniform float shininess = 40.0;
uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(0.5, 0.5, 0.5, 1.0); 
//uniform vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0); 

uniform float falloffScale = 0.3;
//uniform float falloffScale = 0.1;
uniform vec3 thicknessRefraction = vec3(2.0, 2.3, 2.6);
uniform float subSample = 1;

uniform float fresnelBias = 0;
uniform float fresnelScale = 0.4;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct
void main()
{
	// calculate eye-space sphere normal from texture coordinates
	vec3 N;
	N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float r2 = dot(N.xy, N.xy);
	if (r2 > 1.0) discard;   // kill pixels outside circle
	N.z = sqrt(1.0-r2);

	// calculate depth
	vec4 eyeSpacePos = vec4(gl_TexCoord[1].xyz + N*pointRadius, 1.0);   // position of this pixel on sphere in eye space
	vec4 clipSpacePos = gl_ProjectionMatrix * eyeSpacePos;
	gl_FragDepth = (clipSpacePos.z / clipSpacePos.w)*0.5+0.5;

	float diffuse = max(0.0, dot(N, lightDir));

	//gl_FragColor = diffuse*gl_Color;
	gl_FragColor = gl_Color;
	gl_FragColor.w = (1.0-r2)*(1.0-r2)*0.4;


	vec3 v = normalize(-eyeSpacePos.xyz);
	vec3 h = normalize(lightDir + v);
	float specular = pow(max(0.0, dot(N, h)), shininess);

	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(N, v)), fresnelPower);

	// cubemap
	vec3 r = reflect(-v, N);
	vec4 reflectColor = textureCube(cubemapTex, r);

	// color attenuation based on thickness
	float thickness = 0;

	// refraction
	// -----------------
	// Pond
	//vec4 attenuatedColor = fluidColor * exp(-thickness*falloffScale*colorFalloff);
	//float refracMultiplier = 1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// Whirlpool
	vec2 tx = viewportd2 + viewportd2*vec2(clipSpacePos.x/clipSpacePos.w, clipSpacePos.y/clipSpacePos.w);
	vec4 sceneCol = texture2DRect(sceneTex, tx);
	//vec4 sceneCol = texture2DRect(sceneTex, gl_TexCoord[0].xy);
	//sceneCol = vec4(1,1,1,1);''

	//sceneCol.xyz = lerp(vec3(0.13, 0.19, 0.22), sceneCol.xyz, blendFactor);

	
	vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);
	//vec4 finalCol = vec4((1-fresnel)*specularColor.xyz*specular, 1.0);

	float vn = (1-dot(N,v));
	finalCol = vn*vn*gl_Color+specularColor*specular;

	finalCol.w = finalCol.x;
	finalCol.xyz = gl_Color;

	//----------------
	gl_FragColor = finalCol*0.8;

	//gl_FragColor.w = 0.4;
	//gl_FragColor = vec4(clipSpacePos.x/clipSpacePos.w, clipSpacePos.y/clipSpacePos.w, 0, 1);

	//gl_FragColor *= gl_FragColor.w;
}
);

const char *depthBlurViewIDPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
sampler2DRect colorTex;
uniform float world2texScale;
uniform float tex2worldScale;
uniform float maxBlurRadius = 50;
uniform float blurRadiusImagePlane =  10; // in world space
uniform vec2 blurDir; // direction of blur
uniform float XYFalloff = 0.02;
uniform float depthFalloff = 1.00;

void main()
{
	float center = -texture2DRect(colorTex, gl_TexCoord[0].xy).x;
	
	
	if (center < -9999.0) {
		// skip background pixels
		discard;
		return;
	}

	float myWorld2Tex = world2texScale / center;
	float rawRad = blurRadiusImagePlane / center;
	float sampleRadius = round(rawRad);
	sampleRadius = min(sampleRadius, maxBlurRadius);
	sampleRadius = max(0.0, sampleRadius);
	
	float myTex2World = tex2worldScale * center;

	//sampleRadius = max(0.0f, sampleRadius);
	//sampleRadius = 0;
	float sum = 0;
	float wsum = 0;


	for(float i=-sampleRadius; i<=sampleRadius; i+=1) { // step of 1 pixel
		float sample = -texture2DRect(colorTex, gl_TexCoord[0].xy + i*blurDir).x;


		float x = i * myTex2World;

		// bilateral filter
		// spatial domain
		float r = x * XYFalloff;
		float cut = (rawRad - abs(i)) / (rawRad);
		float w = exp(-r*r) * max(0.0f, pow(cut, 0.4)) ;

		// range domain (based on depth difference)
		float r2 = (sample - center) * depthFalloff;
		//float g = 1.0;
		float g = exp(-r2*r2);

		sum += sample * w * g;
		wsum += w * g;
	}

	if (wsum > 0.0) {
		sum /= wsum;
	}
	gl_FragColor.x = -sum;

}
);
const char *depthBlurViewIDNonSepPS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
sampler2DRect colorTex;
uniform float world2texScale;
uniform float tex2worldScale;
uniform float maxBlurRadius = 2;
uniform float blurRadiusImagePlane =  10; // in world space
uniform float XYFalloff = 0.02;
uniform float depthFalloff = 1.00;

void main()
{
	float center = -texture2DRect(colorTex, gl_TexCoord[0].xy).x;
	
	
	if (center < -9999.0) {
		// skip background pixels
		discard;
		return;
	}

	float myWorld2Tex = world2texScale / center;
	float rawRad = blurRadiusImagePlane / center;
	float sampleRadius = round(rawRad);
	sampleRadius = min(sampleRadius, maxBlurRadius);
	sampleRadius = max(0.0, sampleRadius);
	
	float myTex2World = tex2worldScale * center;

	//sampleRadius = max(0.0f, sampleRadius);
	//sampleRadius = 0;
	float sum = 0;
	float wsum = 0;


	for(float i=-sampleRadius; i<=sampleRadius; i+=1) { // step of 1 pixel
		float y = i * myTex2World;
		for(float j=-sampleRadius; j<=sampleRadius; j+=1) { // step of 1 pixel
			float sample = -texture2DRect(colorTex, gl_TexCoord[0].xy + vec2(j, i)).x;

			float x = j * myTex2World;
			float dis = sqrt(x*x+y*y);

			// bilateral filter
			// spatial domain
			float r = dis * XYFalloff;
			float cut = (rawRad - abs(i)) / (rawRad);
			float w = exp(-r*r) * max(0.0f, pow(cut, 0.4)) ;

			// range domain (based on depth difference)
			float r2 = (sample - center) * depthFalloff;
			//float g = 1.0;
			float g = exp(-r2*r2);

			sum += sample * w * g;
			wsum += w * g;
		}
	}

	if (wsum > 0.0) {
		sum /= wsum;
	}
	gl_FragColor.x = -sum;

}
);


// display final shaded surface Nuttapong's mod
const char *displaySurfaceNutEscapePS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform vec2 colorTexScale;
uniform sampler2DArrayShadow stex;
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform sampler2DRect thicknessTex;
uniform sampler2DRect sceneTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.01;
uniform float shininess = 40.0;
//uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);
uniform vec4 fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(0.7, 0.7, 0.7, 1.0); 
//uniform vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0); 

uniform float falloffScale = 1.0f;
//uniform float falloffScale = 0.1;
uniform vec3 thicknessRefraction = vec3(2.0, 2.3, 2.6);
uniform float subSample = 1;

uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct
uniform float shadowAmbient = 0.9;
uniform float splashZShadowBias = 0.0f;
uniform float refracMultiplier = 1.0;

uniform float epsilon = 0.0001f;
uniform float thicknessAlphaMul = 3.0f;
uniform float decayRate = 1.0f;
uniform float thicknessScale = 1.0f;
uniform float thicknessClamp = 1000.0f;
// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
	uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
	return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
	float eyeZ = texture2DRect(tex, texCoord*colorTexScale).x;
	return uvToEye(texCoord*invViewport, eyeZ);
}
float shadowCoef(float3 eyePos)
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(eyePos.x, eyePos.y, eyePos.z,1);
	
	shadow_coord.w = shadow_coord.z + splashZShadowBias;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}

void main()
{
	//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	//return;
	float c = texture2DRect(colorTex, gl_TexCoord[0].xy*colorTexScale).x;
	if (c < -9999.0) {
		discard;
		return;
	}

	// calculate normal
	// taking silohette edges into account
	vec2 uv = gl_TexCoord[0].xy * invViewport;
	vec3 eyePos = uvToEye(uv, c);

	
	vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
	vec3 ddx2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	

	vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
	vec3 ddy2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));

	 float thickness = min(thicknessClamp, texture2DRect(thicknessTex, gl_TexCoord[0].xy * subSample).x* thicknessScale);
	 float refBlend = 0.0f;
	 float maxDz = max(max(abs(ddx.z),abs(ddx2.z)), max(abs(ddy.z),abs(ddy2.z)));
	 if (maxDz > depthThreshold) {
		refBlend = 1.0f - exp((depthThreshold - maxDz)*decayRate);
	 }

//    if (abs(ddx.z) > abs(ddx2.z)) {
	//if (abs(ddx.z) > depthThreshold) { 
		if (abs(ddx2.z) < abs(ddx.z)) {
			ddx = ddx2;
		}
	//}
	//    if (abs(ddy2.z) < abs(ddy.z)) {
	//if (abs(ddy.z) > depthThreshold) { 
		if (abs(ddy2.z) < abs(ddy.z)) {
			ddy = ddy2;
		}	
	//}


	ddx.x += epsilon;
	ddy.y += epsilon;
	

	//vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	//vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);

	// color attenuation based on thickness


	//ddx = normalize(ddx);
	//ddy = normalize(ddy);
	//if (depthThreshold < 0.001f) {
	//	vec3 nn = cross(ddx, ddy);
		//gl_FragColor = vec4(sqrt(dot(ddx,ddx)), sqrt(dot(ddy, ddy)), sqrt(dot(nn,nn)), 1.0f);
	//	gl_FragColor = vec4(10.0f*sqrt(dot(nn,nn)), 0.0f, 0.0f, 1.0f);
	//	return;
	//}
	
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);
	
	vec3 n = normalize(cross(ddx, ddy));


	// lighting
	//    float diffuse = max(0.0, dot(n, lightDir));
	float diffuse = dot(n, lightDir); // wrap lighting

	vec3 v = normalize(-eyePos);
	vec3 h = normalize(lightDir + v);
	float specular = pow(max(0.0, dot(n, h)), shininess);

	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

	// cubemap
	vec3 r = reflect(-v, n);
	r = r * gl_NormalMatrix;
	vec4 reflectColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);//textureCube(cubemapTex, r);

	
	//    vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);

	// refraction
	// -----------------
	// Pond
	//vec4 attenuatedColor = fluidColor * exp(-thickness*falloffScale*colorFalloff);
	//float refracMultiplier = 1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// Whirlpool
	//float refracMultiplier = 0.1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//sceneCol.xyz = lerp(vec3(0.13, 0.19, 0.22), sceneCol.xyz, blendFactor);

	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// FERMI



	float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	vec2 refractedCoord = gl_TexCoord[0].xy + (n.xy * refraction);

    // don't refract objects in front of surface
//	float bgDepth = texture2DRect(sceneDepthTex, refractedCoord).x;
    
  //  if (bgDepth < surfaceDepth) refractedCoord = gl_TexCoord[0].xy;
    //refractedCoord = lerp(refractedCoord, gl_TexCoord[0].xy, smoothstep(0.01, 0.0, bgDepth - surfaceDepth));

    vec4 sceneCol = texture2DRect(sceneTex, refractedCoord);

	
	

	vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);
	sceneCol.xyz = lerp(fluidColor, sceneCol.xyz, attenuatedColor);

	vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + (reflectColor.xyz )*fresnel + specularColor.xyz*specular, 1.0);
	
	gl_FragColor = vec4(diffuse, diffuse, diffuse, 1.0f);//finalCol;
	gl_FragColor = finalCol;
	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * colorTexScale).x;
	
	//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	/*
	float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));

	reflectColor = (1.0f-refBlend)*reflectColor + refBlend*sceneCol;

//	colorFalloff = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff*10.0);
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	//blendFactor = min(0.5, blendFactor);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//fluidColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//fluidColor = vec4(28/255.0, 69/255.0, 89/255.0, 0.0);
	//fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);
	sceneCol.xyz = lerp(fluidColor, sceneCol.xyz, attenuatedColor);
	//sceneCol = fluidColor;
	//sceneCol.xyz = vec3(0.0f,0.0f,0.0f);


	vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel , 1.0);

	float shadow = shadowCoef(eyePos);	
	gl_FragColor = vec4(finalCol.xyz*((1.0f - shadowAmbient)*shadow + shadowAmbient), max(0.0f, min(1.0, thicknessAlphaMul*thickness)));

//	gl_FragColor = finalCol;
//	gl_FragColor.w = max(0.0f, min(1.0, 3.0f*thickness));

	//gl_FragColor = finalCol;
	//    gl_FragColor = sceneCol*attenuatedColor + reflectColor*fresnel + specularColor*specular;

	//    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
	float fog = clamp(gl_Fog.scale*(gl_Fog.end+eyePos.z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(eyePos.z*eyePos.z));

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * colorTexScale).x;
	*/
		//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//return;

	//gl_FragColor.w = max(0.0, min((thickness)*5.0, 1.0));
	//gl_FragColor.w = 1.0;
	
	
}
);

// display final shaded surface Nuttapong's mod
const char *displaySurfaceNutEscapeDiffusePS = STRINGIFY(
//#extension GL_ARB_texture_rectangle : enable\n
uniform vec2 colorTexScale;
uniform sampler2DArrayShadow stex;
uniform sampler2DRect colorTex;
uniform sampler2DRect depthTex;
uniform sampler2DRect thicknessTex;
uniform sampler2DRect sceneTex;
uniform samplerCube cubemapTex;

uniform vec3 lightDir = vec3(0.577, 0.577, 0.577);
uniform vec2 invViewport = vec2(1.0 / 800, 1.0 / 600);
uniform float2 invFocalLen;
uniform float depthThreshold = 0.01;
uniform float shininess = 40.0;
//uniform vec4 fluidColor = vec4(0.5, 0.7, 1.0, 0.0);
uniform vec4 fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);

uniform vec4 colorFalloff = vec4(2.0, 1.0, 0.5, 1.0);
uniform vec4 specularColor = vec4(0.7, 0.7, 0.7, 1.0); 
//uniform vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0); 

uniform float falloffScale = 1.0f;
//uniform float falloffScale = 0.1;
uniform vec3 thicknessRefraction = vec3(2.0, 2.3, 2.6);
uniform float subSample = 1;

uniform float fresnelBias = 0.0;
uniform float fresnelScale = 1.0;
uniform float fresnelPower = 2.0;   // 5.0 is physically correct

uniform float splashZShadowBias = -0.001f;
uniform float refracMultiplier = 1.0;

uniform float epsilon = 0.0001f;
uniform float thicknessAlphaMul = 3.0f;
uniform float decayRate = 1.0f;
uniform float thicknessScale = 1.0f;
uniform float thicknessClamp = 1000.0f;

uniform vec3 spotLightPos;
uniform vec3 spotLightPos2;
uniform vec3 spotLightPos3;
uniform float shadowAmbient = 0.5f;

//#define BLUISH 1
const int BLUISH = 0;
// convert [0,1] uv coords and eye-space Z to eye-space position
vec3 uvToEye(vec2 uv, float eyeZ)
{
	uv = uv * vec2(-2.0, -2.0) - vec2(-1.0, -1.0);
	return vec3(uv * invFocalLen * eyeZ, eyeZ);
}

vec3 getEyePos(sampler2DRect tex, vec2 texCoord)
{
	float eyeZ = texture2DRect(tex, texCoord*colorTexScale).x;
	return uvToEye(texCoord*invViewport, eyeZ);
}

float shadowCoef(float3 eyePos)
{
	const int index = 0;
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(eyePos.x, eyePos.y, eyePos.z,1);
	
	shadow_coord.w = shadow_coord.z + splashZShadowBias;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}

void main()
{
	//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	//return;
	float c = texture2DRect(colorTex, gl_TexCoord[0].xy*colorTexScale).x;
	if (c < -9999.0) {
		discard;
		return;
	}

	// calculate normal
	// taking silohette edges into account
	vec2 uv = gl_TexCoord[0].xy * invViewport;
	vec3 eyePos = uvToEye(uv, c);

	
	vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - eyePos;
	vec3 ddx2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	

	vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - eyePos;
	vec3 ddy2 = eyePos - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));

	 float thickness = min(thicknessClamp, texture2DRect(thicknessTex, gl_TexCoord[0].xy * subSample).x* thicknessScale);
	 float refBlend = 0.0f;
	 float maxDz = max(max(abs(ddx.z),abs(ddx2.z)), max(abs(ddy.z),abs(ddy2.z)));
	 if (maxDz > depthThreshold) {
		refBlend = 1.0f - exp((depthThreshold - maxDz)*decayRate);
	 }

//    if (abs(ddx.z) > abs(ddx2.z)) {
	//if (abs(ddx.z) > depthThreshold) { 
		if (abs(ddx2.z) < abs(ddx.z)) {
			ddx = ddx2;
		}
	//}
	//    if (abs(ddy2.z) < abs(ddy.z)) {
	//if (abs(ddy.z) > depthThreshold) { 
		if (abs(ddy2.z) < abs(ddy.z)) {
			ddy = ddy2;
		}	
	//}


	ddx.x += epsilon;
	ddy.y += epsilon;
	

	//vec3 ddx = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(1, 0)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(-1, 0));
	//vec3 ddy = getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, 1)) - getEyePos(colorTex, gl_TexCoord[0].xy + vec2(0, -1));
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);

	// color attenuation based on thickness


	//ddx = normalize(ddx);
	//ddy = normalize(ddy);
	//if (depthThreshold < 0.001f) {
	//	vec3 nn = cross(ddx, ddy);
		//gl_FragColor = vec4(sqrt(dot(ddx,ddx)), sqrt(dot(ddy, ddy)), sqrt(dot(nn,nn)), 1.0f);
	//	gl_FragColor = vec4(10.0f*sqrt(dot(nn,nn)), 0.0f, 0.0f, 1.0f);
	//	return;
	//}
	
	//ddx.z = clamp(ddx.z, -depthThreshold, depthThreshold);
	//ddy.z = clamp(ddy.z, -depthThreshold, depthThreshold);
	
	vec3 n = normalize(cross(ddx, ddy));

	if (dot(n, eyePos.xyz) > 0) {
		n.xyz *= -1;
	}	

	// lighting
	//    float diffuse = max(0.0, dot(n, lightDir));
	//float diffuse = dot(n, lightDir); // wrap lighting
	vec3 lvec = normalize(spotLightPos - eyePos);
	vec3 lvec2 = normalize(spotLightPos2 - eyePos);
	vec3 lvec3 = normalize(spotLightPos3 - eyePos);

	float shadowC = shadowCoef(eyePos);
	float shadowFactor = ((1.0f - shadowAmbient)*shadowC + shadowAmbient);
	float diffuse = (0.33333f*0.7f*(max(dot(n, lvec),0.0f)+max(dot(n, lvec2),0.0f)+max(dot(n, lvec3),0.0f)))*shadowFactor+0.3f; // wrap lighting

	vec3 v = normalize(-eyePos);
	vec3 h = normalize(lightDir + v);
	float specular = pow(max(0.0, dot(n, h)), shininess);

	float fresnel = fresnelBias + fresnelScale*pow(1.0 - max(0.0, dot(n, v)), fresnelPower);

	// cubemap
	vec3 r = reflect(-v, n);
	r = r * gl_NormalMatrix;
	vec4 reflectColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);//textureCube(cubemapTex, r);

	
	//    vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff);

	// refraction
	// -----------------
	// Pond
	//vec4 attenuatedColor = fluidColor * exp(-thickness*falloffScale*colorFalloff);
	//float refracMultiplier = 1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// Whirlpool
	//float refracMultiplier = 0.1;
	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//sceneCol.xyz = lerp(vec3(0.13, 0.19, 0.22), sceneCol.xyz, blendFactor);

	//vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel + specularColor.xyz*specular, 1.0);

	// FERMI



	//float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	//vec2 refractedCoord = gl_TexCoord[0].xy + (n.xy * refraction);

    // don't refract objects in front of surface
//	float bgDepth = texture2DRect(sceneDepthTex, refractedCoord).x;
    
  //  if (bgDepth < surfaceDepth) refractedCoord = gl_TexCoord[0].xy;
    //refractedCoord = lerp(refractedCoord, gl_TexCoord[0].xy, smoothstep(0.01, 0.0, bgDepth - surfaceDepth));

    

	
	

	
	
	vec3 sceneCol = texture2DRect(sceneTex,  gl_TexCoord[0].xy).xyz;
	vec3 bb = vec3(diffuse,diffuse,diffuse);
	if (BLUISH) {
		vec4 col1 = 1.5*vec4(116,147,179,255) / 200.0;
		bb = col1.xyz*diffuse;
	}
//#endif
	float alpha = clamp(thickness, 0.0f, 1.0f);


	gl_FragColor.xyz = (1.0f-alpha)*sceneCol + alpha*bb;
	gl_FragColor.w = 1.0f;
	//gl_FragColor = finalCol;
	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * colorTexScale).x;
	
	//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

	/*
	float refraction = thickness*thicknessRefraction.x*refracMultiplier;
	vec4 sceneCol = texture2DRect(sceneTex, (gl_TexCoord[0].xy * subSample) + (n.xy * refraction));

	reflectColor = (1.0f-refBlend)*reflectColor + refBlend*sceneCol;

//	colorFalloff = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 attenuatedColor = exp(-thickness*falloffScale*colorFalloff*10.0);
	//vec3 blendFactor = vec3(1,1,1)+2*thickness*falloffScale*vec3(1,1,1);
	//blendFactor = min(0.5, blendFactor);
	
	//blendFactor = min(blendFactor, vec3(1,1,1));
	//blendFactor = max(blendFactor, vec3(0,0,0));
	//fluidColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	//fluidColor = vec4(28/255.0, 69/255.0, 89/255.0, 0.0);
	//fluidColor = vec4(17/255.0, 52/255.0, 71/255.0, 0.0);
	sceneCol.xyz = lerp(fluidColor, sceneCol.xyz, attenuatedColor);
	//sceneCol = fluidColor;
	//sceneCol.xyz = vec3(0.0f,0.0f,0.0f);


	vec4 finalCol = vec4((1-fresnel)*sceneCol.xyz + reflectColor.xyz*fresnel , 1.0);

	float shadow = shadowCoef(eyePos);	
	gl_FragColor = vec4(finalCol.xyz*((1.0f - shadowAmbient)*shadow + shadowAmbient), max(0.0f, min(1.0, thicknessAlphaMul*thickness)));

//	gl_FragColor = finalCol;
//	gl_FragColor.w = max(0.0f, min(1.0, 3.0f*thickness));

	//gl_FragColor = finalCol;
	//    gl_FragColor = sceneCol*attenuatedColor + reflectColor*fresnel + specularColor*specular;

	//    gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy).x;
	float fog = clamp(gl_Fog.scale*(gl_Fog.end+eyePos.z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(eyePos.z*eyePos.z));

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragDepth = texture2DRect(depthTex, gl_TexCoord[0].xy * colorTexScale).x;
	*/
		//gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//return;

	//gl_FragColor.w = max(0.0, min((thickness)*5.0, 1.0));
	//gl_FragColor.w = 1.0;
	
	
}
);

const char *texture2DPS = STRINGIFY(
									uniform sampler2D tex;                                             \n
									void main()                                                        \n
{                                                                  \n
gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);              \n
}                                                                  \n
);

#if TECHNICAL_MODE

const char *particleSmokeUseFOMPS = STRINGIFY(
uniform sampler2DArrayShadow stex;
uniform float shadowAmbient = 0.3;

float shadowCoef()
{
	const int index = 0;
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
uniform float ispotMaxDist;
uniform vec3 spotOriginEye;
uniform sampler2D spot_a0123;
uniform sampler2D spot_b123;

uniform sampler2D smokeTex;

const float PI = 3.1415926535897932384626433832795;
const vec3 _2pik = vec3(2.0) * vec3(PI,2.0*PI,3.0*PI);
const vec3 factor_a = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 factor_b = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 value_1 = vec3(1.0);

uniform mat4 eyeToSpotMatrix;
void main()
{
	//gl_FragColor = gl_Color;
	//return;

    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle

    float falloff = pow(1.0-mag,1.0);//exp(-mag);
	//falloff = 1.0f;
	float shadowC = shadowCoef();
	
	vec3 shadowColor = vec3(0.4, 0.4, 0.4)*0.8;

	// Also FOM
	
//	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	//gl_FragColor.xyz = gl_TexCoord[3].xyz*0.25f;
	//gl_FragColor.xyz = projectionCoordinate.xyz / projectionCoordinate.w;
	//gl_FragColor.w = 1.0f;
	
	//read Fourier series coefficients for color extinction on RGB
	vec4 sR_a0123 = texture2DProj(spot_a0123,projectionCoordinate);
	vec3 sR_b123 = texture2DProj(spot_b123,projectionCoordinate).rgb;

	//gl_FragColor.xyz = sR_a0123.xyz;
	//gl_FragColor.w = 1.0f;
	//return;
	//compute absolute and normalized distance (in spot depth range)
	float distance2spotCenter = length(spotOriginEye-gl_TexCoord[2].xyz);//distance from spot origin to surfel in world space
	float d = distance2spotCenter*ispotMaxDist;

	
	//compute some value to recover the extinction coefficient using the Fourier series
	vec3 sin_a123    = sin(factor_a*vec3(d));
	vec3 cos_b123    = value_1-cos(factor_b*vec3(d));

	//compute the extinction coefficients using Fourier
	float att = (sR_a0123.r*d/2.0) + dot(sin_a123*(sR_a0123.gba/_2pik) ,value_1) + dot(cos_b123*(sR_b123.rgb/_2pik) ,value_1);

	att = max(0.0f, att);
	att = min(1.0f, att);
	shadowC *= (1.0f-att);
	float inS = shadowC;
	shadowC = (shadowAmbient + (1.0f -shadowAmbient)*shadowC);
	//....
	if (gl_TexCoord[0].z > 1) shadowC = 1;
	vec4 texColor = texture2D(smokeTex, gl_TexCoord[0].xy*0.25+gl_TexCoord[1].xy);

    gl_FragColor.xyz = (texColor.x)*gl_Color.xyz*(shadowColor + (vec3(1.0f,1,1) -shadowColor)*shadowC);//*falloff;
	gl_FragColor.w = gl_Color.w*texColor.r;
	

	//float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(gl_TexCoord[0].z*gl_TexCoord[0].z));
	//gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragColor.xyz *= 1.6f;
	gl_FragColor.w *= max(min(falloff,1.0f),0.0f) * max(min(gl_TexCoord[0].w,1.0f),0.0f);
	//gl_FragColor.w = 1;
	//gl_FragColor.xyz = vec3(shadowC, shadowC, shadowC);
//	gl_FragColor.w = 0.2f;
	//gl_FragColor.w = falloff * gl_TexCoord[0].w;
	//gl_FragColor.xyz = sR_a0123.xyz;
	gl_FragColor.xyz *= ((gl_TexCoord[0].z)+inS*0.3)*0.7;
	//gl_FragDepth = gl_FragCoord.z - (1-mag)*0.00002;
	
}
);

#else
const char *particleSmokeUseFOMPS = STRINGIFY(
uniform sampler2DArrayShadow stex;
uniform float shadowAmbient = 0.3;

float shadowCoef()
{
	const int index = 0;
	/*
	int index = 3;
	
	// find the appropriate depth map to look up in based on the depth of this fragment
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
		*/
	
	// transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[2].xyz, 1);

	shadow_coord.w = shadow_coord.z;
	
	// tell glsl in which layer to do the look up
	shadow_coord.z = float(index);

	
	// Gaussian 3x3 filter
	return shadow2DArray(stex, shadow_coord).x;
}
uniform float ispotMaxDist;
uniform vec3 spotOriginEye;
uniform sampler2D spot_a0123;
uniform sampler2D spot_b123;

uniform sampler2D smokeTex;

const float PI = 3.1415926535897932384626433832795;
const vec3 _2pik = vec3(2.0) * vec3(PI,2.0*PI,3.0*PI);
const vec3 factor_a = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 factor_b = vec3(2.0*PI)*vec3(1.0,2.0,3.0);
const vec3 value_1 = vec3(1.0);

uniform mat4 eyeToSpotMatrix;
void main()
{
	//gl_FragColor = gl_Color;
	//return;
/*	
	gl_FragColor = texture2D(smokeTex, gl_TexCoord[0].xy);
	gl_FragColor.w = gl_FragColor.r;
	gl_FragColor.xyz = vec3(1,1,1);
	return;
*/	
//	gl_FragColor = vec4(1,0,0,1);
//	return;
	
    // calculate eye-space normal from texture coordinates
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle

    float falloff = pow(1.0-mag,1.0);//exp(-mag);
	//falloff = 1.0f;
	float shadowC = shadowCoef();
	
	vec3 shadowColor = vec3(0.4, 0.4, 0.9)*0.8;

	// Also FOM
	
//	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	vec4 projectionCoordinate = eyeToSpotMatrix*vec4(gl_TexCoord[2].xyz, 1.0f);
	//gl_FragColor.xyz = gl_TexCoord[3].xyz*0.25f;
	//gl_FragColor.xyz = projectionCoordinate.xyz / projectionCoordinate.w;
	//gl_FragColor.w = 1.0f;
	
	//read Fourier series coefficients for color extinction on RGB
	vec4 sR_a0123 = texture2DProj(spot_a0123,projectionCoordinate);
	vec3 sR_b123 = texture2DProj(spot_b123,projectionCoordinate).rgb;

	//gl_FragColor.xyz = sR_a0123.xyz;
	//gl_FragColor.w = 1.0f;
	//return;
	//compute absolute and normalized distance (in spot depth range)
	float distance2spotCenter = length(spotOriginEye-gl_TexCoord[2].xyz);//distance from spot origin to surfel in world space
	float d = distance2spotCenter*ispotMaxDist;

	
	//compute some value to recover the extinction coefficient using the Fourier series
	vec3 sin_a123    = sin(factor_a*vec3(d));
	vec3 cos_b123    = value_1-cos(factor_b*vec3(d));

	//compute the extinction coefficients using Fourier
	float att = (sR_a0123.r*d/2.0) + dot(sin_a123*(sR_a0123.gba/_2pik) ,value_1) + dot(cos_b123*(sR_b123.rgb/_2pik) ,value_1);

	att = max(0.0f, att);
	att = min(1.0f, att);
	shadowC *= (1.0f-att);
	float inS = shadowC;
	shadowC = (shadowAmbient + (1.0f -shadowAmbient)*shadowC);
	//....
	if (gl_TexCoord[0].z > 1) shadowC = 1;
	vec4 texColor = texture2D(smokeTex, gl_TexCoord[0].xy*0.25+gl_TexCoord[1].xy);

    gl_FragColor.xyz = (texColor.x)*gl_Color.xyz*(shadowColor + (vec3(1.0f,1,1) -shadowColor)*shadowC);//*falloff;
	gl_FragColor.w = gl_Color.w*texColor.r;
	

	//float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[2].z), 0.0, 1.0);
	//float fog = exp(-gl_Fog.density*(gl_TexCoord[0].z*gl_TexCoord[0].z));
	//gl_FragColor = mix(gl_Fog.color, gl_FragColor, fog);

	gl_FragColor.xyz *= 1.6f;
	gl_FragColor.w *= max(min(falloff,1.0f),0.0f) * max(min(gl_TexCoord[0].w,1.0f),0.0f);
	//gl_FragColor.w = 1;
	//gl_FragColor.xyz = vec3(shadowC, shadowC, shadowC);
//	gl_FragColor.w = 0.2f;
	//gl_FragColor.w = falloff * gl_TexCoord[0].w;
	//gl_FragColor.xyz = sR_a0123.xyz;
	gl_FragColor.xyz *= ((gl_TexCoord[0].z)+inS*0.3)*0.7;
	//gl_FragDepth = gl_FragCoord.z - (1-mag)*0.00002;
	
}
);

#endif
// sky box
const char *skyboxVS = STRINGIFY(
								 uniform vec3 projectionCenter = vec3(0,0,0);								  

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	//    gl_Position = gl_Vertex;
	//    gl_Position = gl_ProjectionMatrix * gl_Vertex;
	gl_TexCoord[0].xyz = (gl_Vertex.xyz + projectionCenter);
	//    gl_TexCoord[0].xyz = (mat3) gl_ModelViewMatrix * gl_Vertex.xyz;
	gl_FrontColor = gl_Color;
}
);

const char *skyboxPS = STRINGIFY(
								 uniform samplerCube cubemap;

void main()
{
	gl_FragColor = textureCube(cubemap, gl_TexCoord[0].xyz * vec3(1, 1, 1));

	gl_FragColor.x = 1.5 * pow(gl_FragColor.x, 2.2);
	gl_FragColor.y = 1.5 * pow(gl_FragColor.y, 2.2);
	gl_FragColor.z = 1.5 * pow(gl_FragColor.z, 2.2);
	gl_FragColor.w = 1.5 * pow(gl_FragColor.w, 2.2);
}
);
