uniform sampler2DArrayShadow tex;
uniform float slice;
void main()
{
float v = shadow2DArray(tex, vec4(gl_TexCoord[0].xy,slice,10.001f));
	gl_FragColor = vec4(v,v,v,1);
		//gl_FragColor = vec4(1,0,0,1);
}