    void main(void)
	{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = gl_Vertex * 2.0 - 1.0;
	}