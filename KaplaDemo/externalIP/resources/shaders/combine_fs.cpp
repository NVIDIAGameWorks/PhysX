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
			//return myTexture3D_0(p);
			return vec3(173, 160, 151) *0.85/ 255;
			//return lightwood*1.3;
		}
		else 
		if (mat < 1.5f) {
			//return myTexture3D_1(p);
			return vec3(173, 100, 21)*1.73 / 255;
		} else {
			return vec3(1.0f, 0.0f, 0.0f);
			
		}

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

uniform vec2 shadowTaps[12];


float shadowCoeff1(float bscale)
{	

	int index = 3;
	
	if(gl_FragCoord.z < far_d.x)
		index = 0;
	else if(gl_FragCoord.z < far_d.y)
		index = 1;
	else if(gl_FragCoord.z < far_d.z)
		index = 2;
	
	vec4 shadow_coord = gl_TextureMatrix[index]*vec4(gl_TexCoord[1].xyz, 1);

	shadow_coord.w = shadow_coord.z + shadowAdd*bscale;
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
	const int numTaps = 6;
	float radius = 0.0003f/pow(2,index);
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
	if (dot(bump,bump) < 0.01) bump = vec3(0.5,0.5,1);		
	emissiveReflectSpecPow = vec3(0.0,0.0,0.0);


	// apply bump to the normal
	bump = (bump - vec3(0.5,0.5,0.5)) * 2.0f;
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
		float ldn = max(0.0f, dot(normal, lvec));
		float cosine = dot(lvec, spotLightDir);
		float intensity = smoothstep(spotLightCosineDecayBegin, spotLightCosineDecayEnd, cosine);

		float bscale = 1;//1.0f-ldn;
		
		float shadowC = shadowCoeff1(bscale);
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

	float fog = clamp(gl_Fog.scale*(gl_Fog.end+gl_TexCoord[1].z), 0.0, 1.0);
	vec4 fogCol = gl_Fog.color;
	gl_FragColor = mix(fogCol, gl_FragColor, fog);
}