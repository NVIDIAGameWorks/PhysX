uniform sampler2D diffuseTexture0;
uniform sampler2D diffuseTexture1;

varying lowp vec2 texcoord0;
varying lowp vec4 lighting;
varying lowp vec4 normal;

void main()
{
	// selects texture depending on world normal steepness of height map
	lowp float tex0weight = max(0.0, min(1.0, (normal.y - 0.5) * 3.0));
    gl_FragColor = (texture2D(diffuseTexture0, texcoord0.xy) * tex0weight + texture2D(diffuseTexture1, texcoord0.xy) * (1.0 - tex0weight)) * lighting;
}
