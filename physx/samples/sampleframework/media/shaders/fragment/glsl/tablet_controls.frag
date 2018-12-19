uniform sampler2D diffuseTexture;
varying lowp vec2 texcoord0;

void main()
{
    gl_FragColor = texture2D(diffuseTexture, texcoord0.xy);
}