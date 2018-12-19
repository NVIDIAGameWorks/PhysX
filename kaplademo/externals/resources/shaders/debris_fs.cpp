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

vec3 myTexture3D(vec3 p)
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
		diffuseMat = myTexture3D(gl_TexCoord[0].xyz) * vec3(0.5,0.5,0.5);//texture3D(ttt3D, gl_TexCoord[0].xyz);
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

	vec3 eyeVec = normalize(gl_TexCoord[1].xyz);

	// apply gamma correction for diffuse textures
	diffuseMat = pow(diffuseMat, 0.45);

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
	//color += hdrScale * emissive * diffuseMat;
	
	//vec3 reflectColor = diffuseMat * texture2DRect(reflectionTex, gl_FragCoord.xy).rgb;
	//color = reflectionCoeff * reflectColor + (1.0f - reflectionCoeff) * color;
	//color = (fresnel * skyColor + (1.0 - fresnel) * color) * reflectivity + (1.0 - reflectivity) * color;

	gl_FragColor.rgb = color;
	gl_FragColor.w = gl_Color.w;
	
	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);
}