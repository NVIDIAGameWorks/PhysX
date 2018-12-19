
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
	
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
	
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

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
	const int numTaps = 12;
	float radius = 0.0003f / pow(2, index);
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
	
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

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
	
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

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
	emissiveReflectSpecPow = vec3(0.0,0.0,0.0);

	vec3 normal = normalize(gl_TexCoord[2].xyz);
	vec3 wnormal = normalize(gl_TexCoord[4].xyz);
	// read in material color for diffuse, specular, bump, emmisive

	// 3D texture	
	vec4 colorx;
	if (useTexture > 0)
		colorx = texture2D(texture, gl_TexCoord[0]);
	else {
		colorx = gl_Color;
		colorx *= 1.0 - 0.25*checker(float2(gl_TexCoord[3].x, gl_TexCoord[3].z));
	}	
	colorx = clamp(colorx,0,1);
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
		diffuseColor += (1.0 - shadowC*ldn) * shadowAmbient * shadowColor * diffuseMat*intensity;

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
	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);

}