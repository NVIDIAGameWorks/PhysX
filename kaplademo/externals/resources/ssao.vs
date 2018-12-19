uniform vec2 scaleXY;
void main(void)
{
	gl_TexCoord[0] = gl_Vertex;
	gl_TexCoord[1] = vec4(gl_Vertex.xy * scaleXY, gl_Vertex.zw);
	gl_Position = gl_Vertex * 2.0f - 1.0f;
}
