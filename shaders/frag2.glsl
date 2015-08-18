#version 450
#ifdef GL_ES
precision highp float;
#endif

#pragma optionNV(unroll all)

out vec4 color;
in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform float time;
uniform vec2 size;

uniform sampler2D tex;

vec3 hsv(in float h, in float s, in float v)
{
	return mix(vec3(1.0), clamp((abs(fract(h + vec3(3, 2, 1) / 3.0) * 6.0 - 3.0) - 1.0), 0.0 , 1.0), s) * v;
}

void main()
{
	float peaktime = time + peak / 10000.0;

	vec4 prev = texture(tex, texCoord) * transition;

	float invtrans = 1.0 - transition;

	vec3 col = vec3(0.0);
	vec2 c = (time / 50.0) * vec2(0.065, -0.0534);
	vec2 p = texCoord * 2.0 - 1.0;
	p.x *= size.x / size.y;
	p *= 2.0;

	float r = smoothstep(0.0, 0.2, sqrt(dot(p, p)));

	float dotp = dot(p, p);
	for(int i = 0; i < 5; i++){
		p = abs(mod(p / dotp + c, 2.0) - 1.0);

		dotp = dot(p, p);
		float sqrp = sqrt(dotp);
		r *= smoothstep(0.0, 0.2, sqrp);
		col += hsv(1.0 - max(p.x, p.y) * sqrp + peaktime / 200.0 + 2.0, 1.0 - sqrp + prev.r + prev.g, r);
	}

	col = -cos(col) * 0.5 + 0.5;
	
	color = vec4((1.0 - col) * invtrans, 1.0) + prev;
}
