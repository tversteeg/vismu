#version 450
#ifdef GL_ES
precision highp float;
#endif

#pragma optionNV(unroll all)

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;
in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform float time;

uniform sampler2D tex;

vec3 hsv(in float h, in float s, in float v)
{
	return mix(vec3(1.0), clamp((abs(fract(h + vec3(3, 2, 1) / 3.0) * 6.0 - 3.0) - 1.0), 0.0 , 1.0), s) * v;
}

vec2 distanceToObj(in vec3 point)
{
	return vec2(point.y + 3.0, 0.0);
}

void main()
{
	vec4 prev = texture(tex, texCoord);
	prev.rgb *= vec3(transition);

	float invtrans = 1.0 - transition;

	// Camera
	vec2 texscreen = texCoord * 2.0 - 1.0;

	vec3 c = vec3(0.0);
	vec2 p = texCoord * sin(time / 100.0) + 1.0;
	float dotp = dot(p, p);

	for(int i = 0; i < 2; i++){
		p = abs(mod(p / dotp, 2.0) - 1.0);

		dotp = dot(p, p);
		float sqrp = sqrt(dotp);

		c += hsv(1.0 - max(p.x, p.y) * sqrp, 2.0 - sqrp, smoothstep(0.0, 0.2, sqrp));
	}

	c = -cos(c) * 0.5 + 0.5;
	
	color = vec4(c * invtrans, 1.0) + prev;
}
