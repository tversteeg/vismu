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

void main()
{
	vec4 prev = texture(tex, texCoord) * transition;

	vec4 cur = vec4(texCoord.x, texCoord.y, peak / 100000.0, 1.0);

	cur *= 1.0 - transition;
	
	color = cur + prev;
}
