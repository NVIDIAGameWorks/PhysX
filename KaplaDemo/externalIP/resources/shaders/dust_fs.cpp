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
