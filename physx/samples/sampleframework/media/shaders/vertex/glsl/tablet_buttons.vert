attribute vec3 position_attr;

void main()
{
    gl_Position = vec4(position_attr, 1.0);
}