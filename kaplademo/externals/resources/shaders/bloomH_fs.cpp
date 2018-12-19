	uniform sampler2D colorTex;
	uniform float sx;

	void main (void)
	{
		vec3 bloom = vec3(0.0, 0.0, 0.0);
		const float hdrScale = 1.5;
		const int kernelSize = 10;
		const float invScale = 1.0 / (hdrScale * float(kernelSize));

		for (int x = -kernelSize; x <= kernelSize; x++)
		{
			float s = gl_TexCoord[0].s + x * sx;
			float t = gl_TexCoord[0].t;
			vec3 color = texture2D(colorTex, vec2(s,t)).rgb;
			float luminance = dot(color, vec3(0.2125, 0.7154, 0.0721));
			if (luminance > 1.0)
			{
				bloom += color * ((kernelSize+1) - abs(float(x)));
			}
		}

		gl_FragColor = vec4(bloom * invScale, 1.0);
	}