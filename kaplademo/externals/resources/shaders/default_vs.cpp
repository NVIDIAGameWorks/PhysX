uniform float uvScale = 1.0f;
void main()
{
	vec4 eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal.xyz,0.0); 
	gl_TexCoord[3].xyz = gl_Vertex.xyz;
	gl_TexCoord[4].xyz = gl_Normal.xyz;
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}