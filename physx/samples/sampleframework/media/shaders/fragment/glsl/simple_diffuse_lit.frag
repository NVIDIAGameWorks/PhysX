uniform sampler2D diffuseTexture;

varying lowp vec2 texcoord0;
varying lowp vec4 lighting;

void main()
{
    gl_FragColor = texture2D(diffuseTexture, texcoord0.xy) * lighting;
}
