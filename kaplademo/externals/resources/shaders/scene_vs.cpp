
uniform float uvScale = 1.0f;
uniform sampler2D transTex;
uniform int transTexSize;
uniform float iTransTexSize;
uniform float bumpTextureUVScale;
//attribute mat4 transformmatrix;
void main()
{
	
	int ti = (int)(gl_MultiTexCoord0.w);
	//int ti = tq;
	int tpr = transTexSize / 4;
	int row = ti / tpr;
	int col = (ti - row*tpr)*4;

	float fx = (col+0.5f)*iTransTexSize;
	float fy = (row+0.5f)*iTransTexSize;

	
	vec4 r0 = texture2D(transTex, vec2(fx,fy));
	vec4 r1 = texture2D(transTex, vec2(fx+iTransTexSize,fy));
	vec4 r2 = texture2D(transTex, vec2(fx+iTransTexSize*2.0f,fy));
	vec4 r3 = texture2D(transTex, vec2(fx+iTransTexSize*3.0f,fy));
//	vec4 r3 = vec4(0,0,0,1);
	
	vec3 offset = vec3(r0.w, r1.w, r2.w);
	r0.w = 0.0f;
	r1.w = 0.0f;
	r2.w = 0.0f;

	float material = r3.w;
	r3.w = 1.0f;
	mat4 transformmatrix = mat4(r0,r1,r2,r3);


	

	mat4 mvp = gl_ModelViewMatrix * transformmatrix;
	mat4 mvpt = gl_ModelViewMatrixInverseTranspose * transformmatrix;
	vec4 t0 = vec4(gl_MultiTexCoord0.xyz, 0.0f);

	vec4 t1 = vec4(cross(gl_Normal.xyz, t0.xyz), 0.0f);

//	mat4 mvp = gl_ModelViewMatrix;
//	mat4 mvpt = gl_ModelViewMatrixInverseTranspose;


	vec4 eyeSpacePos = mvp * gl_Vertex;
	//eyeSpacePos.y += gl_InstanceID * 0.2f;
	//gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz*uvScale;
	vec3 coord3d = gl_Vertex.xyz + offset;
	gl_TexCoord[0].xyz = (coord3d)*uvScale;
	gl_TexCoord[1] = eyeSpacePos;
	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix*eyeSpacePos;
	gl_TexCoord[2] = mvpt * vec4(gl_Normal.xyz,0.0); 

	gl_TexCoord[3] = mvpt * t0;
	gl_TexCoord[4].xyz = mvpt * t1;

	gl_TexCoord[5].xy = vec2(dot(coord3d,  t0.xyz), dot(coord3d,  t1.xyz))*bumpTextureUVScale*2;
	
	gl_TexCoord[6].xyz = vec3(gl_MultiTexCoord1.xy, material);
	gl_TexCoord[6].y = 1.0 - gl_TexCoord[6].y;

	float MAX_3D_TEX = 8.0;
	if (gl_TexCoord[6].x >= 5.0f) {
		// 2D Tex
		gl_TexCoord[6].x -= 5.0f;		
		gl_TexCoord[6].z = floor(gl_TexCoord[6].z / MAX_3D_TEX);
	} else {
		gl_TexCoord[6].z -= floor(gl_TexCoord[6].z / MAX_3D_TEX)*MAX_3D_TEX;
		gl_TexCoord[6].z -= 100.0f;
		
	}

	gl_ClipVertex = vec4(eyeSpacePos.xyz, 1.0f);
}
