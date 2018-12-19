//#version 120\n
//#extension GL_EXT_geometry_shader4 : enable\n
uniform float pointRadius;  // point size in world space
uniform float densityThreshold = 50.0;
uniform float idensityThreshold = 1.0 / 30.0;
uniform float pointShrink = 0.25;
uniform sampler2D meteorTex;
void main()
{
    gl_FrontColor = gl_FrontColorIn[0];
    float density = gl_TexCoordIn[0][1].x;
	float life = gl_TexCoordIn[0][1].y;
	

	gl_TexCoord[1].xy = 0.25f*vec2(gl_PrimitiveIDIn / 4, gl_PrimitiveIDIn % 4);
    // scale down point size based on density
	float factor = 1.0f;//density * idensityThreshold;
	//smoothstep(0.0f, densityThreshold, density);
		//density * idensityThreshold;
		//clamp(density / 50.0f, 0, 1);
    float pointSize = pointRadius*factor;//*(pointShrink + smoothstep(0.0, densityThreshold, density)*(1.0-pointShrink));

	pointSize *= gl_TexCoordIn[0][3].x;
	float tmp = gl_TexCoordIn[0][3].y;

	float bb = 1.0f;
	if (tmp > 0.5f) {
		//gl_FrontColor = vec4(3*life,0,0,1);
		// TODO: Meteor trail color here...
		//vec2 fetchPos = vec2( min(max((3-lifeTime)/3,0),1), 0);
		float val = 1-min(max((life-0.3)/0.2,0.01),0.99);
		vec2 fetchPos = vec2(val, 0);
		gl_FrontColor = texture2D(meteorTex, fetchPos);
		if (gl_FrontColor.r > 0.5) bb += (gl_FrontColor.r-0.5)*(gl_FrontColor.r-0.5)*10;
		
	}
//    float pointSize = pointRadius;

    // eye space
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 pos2 = gl_TexCoordIn[0][0].xyz;
    vec3 motion = pos - pos2;
    vec3 dir = normalize(motion);
    float len = length(motion);

    vec3 x = dir * pointSize;
    vec3 view = normalize(-pos);
    vec3 y = normalize(cross(dir, view)) * pointSize;
    float facing = dot(view, dir);

    // check for very small motion to avoid jitter
    float threshold = 0.01;
//    if (len < threshold) {
    if ((len < threshold) || (facing > 0.95) || (facing < -0.95)) {
        pos2 = pos;
        x = vec3(pointSize, 0.0, 0.0);
        y = vec3(0.0, -pointSize, 0.0);
    }

	float angle = density;
	float cv = cos(angle);
	float sv = sin(angle);

	vec3 xt = cv*x + sv*y;
	vec3 yt = -sv*x + cv*y;
	x = xt;
	y = yt;

    {

        gl_TexCoord[0] = vec4(0, 0, bb, life);
        gl_TexCoord[2] = vec4(pos + x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
		gl_TexCoord[3] = gl_TexCoordIn[0][2];
        EmitVertex();

        gl_TexCoord[0] = vec4(0, 1, bb, life);
		gl_TexCoord[2] = vec4(pos + x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();

        gl_TexCoord[0] = vec4(1, 0, bb, life);
		gl_TexCoord[2] = vec4(pos2 - x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();

        gl_TexCoord[0] = vec4(1, 1, bb, life);
        gl_TexCoord[2] = vec4(pos2 - x - y, 1);
		gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];

        EmitVertex();
/*
        gl_TexCoord[0] = vec4(0, 0, 0, life);
        gl_TexCoord[2] =  vec4(pos + x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(0, 1, 0, life);
        gl_TexCoord[2] = vec4(pos + x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 0, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x + y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();

        gl_TexCoord[0] = vec4(1, 1, 0, life);
        gl_TexCoord[2] = vec4(pos2 - x - y, 1);
        gl_Position = gl_ProjectionMatrix * gl_TexCoord[2];
        EmitVertex();
		*/
    }
}
