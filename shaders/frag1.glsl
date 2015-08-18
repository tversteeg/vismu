#version 450
#ifdef GL_ES
precision highp float;
#endif

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;
in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform float time;

uniform sampler2D tex;

float hash(vec2 p)
{
	float h = dot(p, vec2(127.1,311.7));

	return -1.0 + 2.0 * fract(sin(h) * 43758.5453123);
}

float noise(in vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);

	vec2 u = f * f * (3.0 - 2.0 * f);

	return mix(	mix(	hash(i + vec2(0.0,0.0)), 
										hash(i + vec2(1.0,0.0)), u.x), 
							mix(	hash(i + vec2(0.0,1.0)), 
										hash(i + vec2(1.0,1.0)), u.x), u.y);
}

void main()
{
	vec4 prev = texture(tex, texCoord) * transition;

	vec4 cur = vec4(texCoord.x, texCoord.y, peak / 100000.0, 1.0);

	float noiseval = 0.5 + 0.5 * noise(texCoord * 64.0);

	cur.rgb *= vec3(noiseval);

	cur *= 1.0 - transition;

	color = cur + prev;
}
