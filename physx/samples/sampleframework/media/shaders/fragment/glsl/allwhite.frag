uniform lowp vec4 diffuseColor;

varying lowp vec4 lighting;

void main()
{
    gl_FragColor = diffuseColor * lighting;
}