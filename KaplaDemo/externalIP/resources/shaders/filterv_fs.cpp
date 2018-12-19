	uniform sampler2D ssaoTex;
	uniform float sy;

	void main (void)
	{
		
		float SSAO = 0.0;

		for(int y = -4; y <= 4; y++)
		{
			SSAO += texture2D(ssaoTex,vec2(gl_TexCoord[0].s,y * sy + gl_TexCoord[0].t)).r * (5.0 - abs(float(y)));
		}

		gl_FragColor = vec4(vec3(pow(SSAO / 25.0,1.5)),1.0);
		gl_FragColor.w = gl_FragColor.x;
		//gl_FragColor = vec4(1,1,1,1);
	}