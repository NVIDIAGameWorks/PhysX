#define SAMPLEDIV 4
#define NUMSAMPLES SAMPLEDIV*SAMPLEDIV
#define iNUMSAMPLES 1.0f / (SAMPLEDIV * SAMPLEDIV)
#define BIAS 1.0f

uniform vec3 samples[NUMSAMPLES];
uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D unitVecTex;
uniform mat4x4 biasProjMat;
uniform mat4x4 biasProjMatInv;

void main (void)
{
	float ssao = 0.0;
	float depth = texture2D(depthTex, gl_TexCoord[0].st).r;
	if(depth < 1.0)
	{
		vec3 dir = normalize(texture2D(unitVecTex, gl_TexCoord[1].st).rgb * 2.0f - 1.0f);
		vec3 n = normalize(texture2D(normalTex, gl_TexCoord[0].st).rgb * 2.0f - 1.0f);
		vec4 myPosEye = biasProjMatInv * vec4(gl_TexCoord[0].st, depth, 1.0f);
		myPosEye.xyz /= myPosEye.w;
		vec3 t0 = normalize(dir - n * dot(dir, n));
		vec3 t1 = cross(n, t0);
		mat3x3 rmat = mat3x3(t0, t1, n);
		for(int i = 0; i < NUMSAMPLES; i++)
		{
			vec3 samplePosEye = rmat * samples[i] + myPosEye.xyz;
			vec4 samplePosScreen = biasProjMat * vec4(samplePosEye, 1.0f);
			samplePosScreen.xyz /= samplePosScreen.w;
			float sampleDepth = texture2D(depthTex, samplePosScreen.st).r;
			if(sampleDepth < samplePosScreen.z)
			{
				vec4 surfacePosWorld = biasProjMatInv * vec4(samplePosScreen.st, sampleDepth, 1.0f);
				surfacePosWorld.xyz /= surfacePosWorld.w;		
				vec3 p2p = surfacePosWorld.xyz - myPosEye.xyz;				
				ssao += 1.0f / (BIAS+dot(p2p, p2p));
			}
		}
		ssao = 1.0f - ssao * iNUMSAMPLES;
	}	else {
		ssao = 1.0f;
	}
	gl_FragColor = vec4(ssao,ssao,ssao,1.0f);
}
