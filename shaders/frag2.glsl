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
	vec4 prev = texture(tex, texCoord) * (transition * transition);

	float invtrans = 1.0 - transition;

	float peaktime = time + peak / 10000.0;

	vec3 col = vec3(0.0);
	vec2 c = (time / 50.0) * vec2(0.065, -0.0534) * invtrans;
	vec2 p = texCoord * 2.0 - 1.0;
	p.x *= 800.0 / 600.0;
	p *= 2.0;

	float r = smoothstep(0.0, 0.2, sqrt(dot(p, p)));

	float dotp = dot(p, p);
	for(int i = 0; i < 5; i++){
		p = abs(mod(p / dotp + c, 2.0) - 1.0);

		dotp = dot(p, p);
		float sqrp = sqrt(dotp);
		r *= smoothstep(0.0, 0.2, sqrp);
		col += hsv(1.0 - max(p.x, p.y) * sqrp + peaktime / 120.0 + 2.0, 1.0 - sqrp, r);
	}

	col = 0.5 - cos(col) * 0.5 * invtrans;
	
	color = vec4(col * invtrans, 1.0) + prev;
}
