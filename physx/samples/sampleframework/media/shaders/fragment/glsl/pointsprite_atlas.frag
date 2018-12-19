uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
varying highp vec2 texcoord0;
varying highp vec4 color;

void main()
{
  	highp vec2 uv = vec2(texcoord0.x, 1.0-texcoord0.y);
    highp float tcos  = color.z;
	highp float tsin  = color.w;
	
	uv -= 0.5;
	highp vec2 tempuv;
	tempuv.x = uv.x*tcos - uv.y*tsin;
	tempuv.y = uv.x*tsin + uv.y*tcos;
	uv = tempuv;
	uv += 0.5;
	
	uv  = clamp(uv, 0.0, 1.0);
	uv *= 0.5;
	uv += color.xy;

    if (texture2D(normalTexture, uv.xy).a <= 0.5)
        discard;
	gl_FragColor.rgb = texture2D(diffuseTexture, uv.xy).rgb;
}
