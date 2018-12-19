uniform float timestep = 0.02;
uniform vec3 eyeVel;
uniform float iStartFade = 1.0;
void main()
{
    vec3 pos = gl_Vertex.xyz;
    vec3 vel = gl_MultiTexCoord2.xyz;
	//vel = vec3(10.0f,0.0f,0.0f);
    vec3 pos2 = (pos - (vel+eyeVel)*timestep);   // previous position

    gl_Position = gl_ModelViewMatrix * vec4(pos, 1.0);     // eye space
    gl_TexCoord[0] = gl_ModelViewMatrix * vec4(pos2, 1.0);
    gl_TexCoord[1].x = gl_MultiTexCoord1.x;
	gl_TexCoord[1].y = max(0.0f, min(gl_MultiTexCoord3.x*iStartFade, 1.0f));
	gl_TexCoord[2].xyz = pos;
	gl_TexCoord[3] = gl_MultiTexCoord4;

    gl_FrontColor = gl_Color;
}
