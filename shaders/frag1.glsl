#version 300 es
precision mediump float;

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;

in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform sampler2D tex;

void main()
{
	vec4 prev = texture(tex, texCoord) * transition;

//	vec4 cur = vec4(texCoord.x, texCoord.y, peak / 50000.0, 1.0) * 1.0 - transition;
	vec4 cur = vec4(0.0, 1.0 - transition, 0.0, 1.0);

	color = cur + prev;
}
