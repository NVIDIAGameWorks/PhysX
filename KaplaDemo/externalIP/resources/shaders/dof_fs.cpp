	uniform sampler2D colorTex;
	uniform sampler2D depthTex;
	uniform float sx;
	uniform float sy;

	void main (void)
	{
		const float depthEnd = 0.993;
		const float depthSize = 0.015;

		vec3 colorP = texture2D(colorTex, gl_TexCoord[0]).rgb;
		float depth = texture2D(depthTex, gl_TexCoord[0].st).r;

		if ((depth - depthEnd) < depthSize)
		{
			const int depthKernelSize = 5;
			vec3 colorSum = vec3(0.0);
			float cnt = 0.0;
			for (int x = -depthKernelSize; x <= depthKernelSize; x++)
				for (int y = -depthKernelSize; y <= depthKernelSize; y++)
				{
					float s = gl_TexCoord[0].s + x * sy;
					float t = gl_TexCoord[0].t + y * sy;
					float scalex = ((depthKernelSize+1) - abs(float(x))) / depthKernelSize;
					float scaley = ((depthKernelSize+1) - abs(float(y))) / depthKernelSize;
					float scale = scalex * scaley;
					vec3 color = texture2D(colorTex, vec2(s,t)).rgb;
					colorSum += scale * color;
					cnt += scale;
				}

			colorSum /= cnt;
			float depthScale = pow(max(0.0f,min(1.0, ( abs(depth-depthEnd)) / depthSize)),1.5);
			colorP = depthScale * colorSum + (1.0 - depthScale) * colorP;
		}

		gl_FragColor = vec4(colorP, 1.0);
	}