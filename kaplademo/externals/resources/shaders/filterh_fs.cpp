	uniform sampler2D ssaoTex;
	uniform float sx;

	void main (void)
	{
		float SSAO = 0.0;

		for(int x = -4; x <= 4; x++)
		{
			SSAO += texture2D(ssaoTex,vec2(x * sx + gl_TexCoord[0].s,gl_TexCoord[0].t)).r * (5.0 - abs(float(x)));
		}

		gl_FragColor = vec4(vec3(SSAO / 25.0),1.0);
		gl_FragColor.w = gl_FragColor.x;
	}