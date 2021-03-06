#version 450
#ifdef GL_ES
precision highp float;
#endif

out vec4 color;
in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform float time;
uniform vec2 size;

uniform sampler2D tex;

void main()
{
	float peaktime = time + peak * 0.001;

	vec4 prev = texture(tex, texCoord) * transition;

	float invtrans = 1.0 - transition;

	vec2 uv = texCoord * 2.0 - 1.0;
	uv.x *= size.x / size.y;
	uv *= sin(time / 200.0) * 0.8 + 1.0;

	vec3 tot = vec3(0.0);

	float sintime = sin(peaktime * 0.001) * 0.2 + 0.5;
	vec4 p = vec4(uv, sintime, 1.0);
	vec4 p2 = vec4(uv, sintime, 0.0);
	vec4 p3 = vec4(uv, 0.5, 0.5);

	float fractime = 0.2 + cos(peaktime / 2000.0) * 0.1 + (prev.r * transition * 0.2);
	for(int i = 0; i < 16; i++){
		float len = length(p);
		p = abs(p) / (len * len) - fractime;
		tot.r += abs(length(p) - len);

		len = length(p2);
		p2 = abs(p2) / (len * len) - fractime + 0.1;
		tot.g += abs(length(p2) - len);

		len = length(p3);
		p3 = abs(p3) / (len * len) - fractime + 0.05;
		tot.b += abs(length(p3) - len);
	}

	vec3 b = -cos(tot / 3.0) * 0.5 + 0.5;
	vec4 col = vec4(b, 1.0);

	col *= invtrans;

	color = col + prev;
}
