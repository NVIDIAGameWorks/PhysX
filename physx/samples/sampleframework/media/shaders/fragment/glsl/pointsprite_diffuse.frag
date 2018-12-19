uniform sampler2D diffuseTexture;
uniform highp vec4 diffuseColor;

void main()
{
	gl_FragColor = diffuseColor * texture2D(diffuseTexture, gl_PointCoord);
}
