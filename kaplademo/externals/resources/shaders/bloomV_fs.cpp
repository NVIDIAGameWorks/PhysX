	uniform sampler2D colorTex;
	uniform sampler2D blurTex;
	uniform float sy;

	void main (void)
	{
		const float hdrScale = 1.5;
		const int kernelSize = 10;
		const float invScale = 1.0 / (hdrScale * float(kernelSize) * 100.0);

		vec3 colorP = texture2D(colorTex, gl_TexCoord[0]).rgb;
		vec3 bloom = vec3(0.0, 0.0, 0.0);

		for (int y = -kernelSize; y <= kernelSize; y++)
		{
			float s = gl_TexCoord[0].s;
			float t = gl_TexCoord[0].t + y * sy;
			vec3 color = texture2D(blurTex, vec2(s,t)).rgb;
			float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
			if (luminance > 1.0)
			{
				bloom += color  * ((kernelSize+1) - abs(float(y)));
			}
		}

		vec3 hdrColor = invScale * bloom + colorP;

		vec3 toneMappedColor = 2.0 * hdrColor / (hdrColor + vec3(1.0));
		gl_FragColor = vec4(toneMappedColor, 1.0);
	}