#version 300 es
precision mediump float;

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;

in vec2 texCoord;

uniform float peak;

void main()
{
	color = vec4(texCoord.x, texCoord.y, peak / 50000.0, 1.0);	
}
