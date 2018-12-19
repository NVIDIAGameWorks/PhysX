
uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;

varying highp vec2 texcoord0;

void main()
{
    lowp vec4 diffuseTextureColor = texture2D(diffuseTexture, texcoord0.xy);
	lowp vec4 normalTextureColor  = texture2D(normalTexture,  texcoord0.xy);
    
    gl_FragColor = 	clamp(normalTextureColor.z, 0.2, 1.0) * diffuseTextureColor;
}
