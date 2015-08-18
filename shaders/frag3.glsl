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

float fractalNoise(in vec2 p, float maxval)
{
	mat2 mat = mat2(1.6, 1.2, -1.2, 1.6);
	float noiseval = 0;

	for(float i = 1.0; i < maxval; i *= 2.0){
		noiseval += (1.0 / i) * noise(p);
		p *= mat;
	}

	noiseval = noiseval * 0.5 + 0.5;

	return smoothstep(1.0, 0.0, noiseval);
}

void main()
{
	vec4 prev = texture(tex, texCoord / transition) * transition;

	vec2 texscreen = texCoord * 2.0 - 1.0;
	texscreen.x *= size.x / size.y;

	//texscreen *= sin(time / 1000.0) * 10.0 + 10.0;
	texscreen *= 10.0;

	const float iter = 50.0;

	vec2 pos = vec2(sin(time / 50.0), cos(time / 50.0)) * 5.0;

	vec2 texc = texscreen;
	vec2 delta = (texscreen - pos) / iter * 0.8;
	float illumination = 1.0;

	vec3 col = vec3(fractalNoise(texc, 32.0) * 0.2);
	for(int i = 0; i < iter; i++){
		texc -= delta * noise(texc);
		float fracn = (1.0 - fractalNoise(texc, 32.0)) * 0.2;
		
		col += fracn * illumination * 0.3;
		illumination *= 0.8;
	}

	color = vec4(col, 1.0) + prev;
}
