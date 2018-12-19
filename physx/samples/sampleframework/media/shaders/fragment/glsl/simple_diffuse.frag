uniform sampler2D diffuseTexture;

varying highp vec2 texcoord0;
varying lowp vec4 color;

void main()
{
    gl_FragColor = texture2D(diffuseTexture, texcoord0.xy);
}
