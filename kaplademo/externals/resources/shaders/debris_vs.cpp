uniform float uvScale = 1.0f;
attribute mat4 transformmatrix;
uniform float bumpTextureUVScale;

void main()
{
	mat4 mvp = gl_ModelViewMatrix * transformmatrix;
	mat4 mvpt = gl_ModelViewMatrixInverseTranspose * transformmatrix;
	//mat4 mvp2 = gl_ModelViewMatrix * transformmatrix;
	//mat4 mvp = gl_ModelViewMatrix;
	//mat4 mvpt = gl_ModelViewMatrixInverseTranspose;	

	vec4 eyeSpacePos = mvp * gl_Vertex;

	vec4 t0 = vec4(gl_MultiTexCoord0.xyz, 0.0f);
	vec4 t1 = vec4(cross(gl_Normal.xyz, t0.xyz), 0.0f);


	vec3 coord3d = gl_Vertex.xyz;
	gl_TexCoord[0].xyz = (coord3d)*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 

	gl_TexCoord[3] = mvpt * t0;
	gl_TexCoord[4] = mvpt * t1;

	gl_TexCoord[5].xy = vec2(dot(coord3d,  t0.xyz), dot(coord3d,  t1.xyz))*bumpTextureUVScale*2;

	gl_TexCoord[6].xyz = vec3(0,0,-100); // TODO: 2D UV are 0 and material id is -100 (first 3D texture)

	/*
	//vec4 eyeSpacePos2 = mvp2 * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	//gl_FrontColor.x += eyeSpacePos2.x;
	
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 
	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
	*/
}