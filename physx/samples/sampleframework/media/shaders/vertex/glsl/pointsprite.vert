uniform mat4 g_viewMatrix;
uniform mat4 g_modelMatrix;
uniform mat4 g_projMatrix;

attribute float psize;
attribute vec3 position_attr;

void main()
{	
	gl_Position  = g_projMatrix * g_viewMatrix * vec4(position_attr, 1.0);
	gl_PointSize = (700.0 / gl_Position.w);
}

