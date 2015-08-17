#version 300 es
precision mediump float;

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;

in vec2 texCoord;

uniform float peak;

void main()
{
	color = vec4(1.0 - texCoord.y, texCoord.x, 1.0 - peak / 50000.0, 1.0);	
}
