#version 300 es
precision mediump float;

#define screenX 1.0/800
#define screenY 1.0/600

out vec4 color;
in vec2 texCoord;

uniform float peak;
uniform float transition;
uniform float time;

uniform sampler2D tex;

vec2 distanceToObj(in vec3 point)
{
	return vec2(point.x + 10.0, 0.0);
}

void main()
{
	vec4 prev = texture(tex, texCoord / transition) * transition;

	// Camera
	vec2 texscreen = texCoord * 2.0 - 1.0;

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 lookat = vec3(0.0, 0.0, 0.0);
	vec3 cam = vec3(sin(peak) * 8.0, 4.0, 0.0);

	vec3 p = normalize(lookat - cam);
	vec3 u = normalize(cross(up, p));
	vec3 v = cross(p, u);
	vec3 cv = cam + p;
	vec3 screen = cv + texscreen.x * u * 0.8 + texscreen.y * v * 0.8;
	vec3 screenp = normalize(screen - cam);

	const vec3 e = vec3(0.02, 0.0, 0.0);
	const float maxdepth = 500.0;
	vec2 dis = vec2(0.02, 0.0);
	
	vec3 result, point;
	float f = 1.0;
	for(int i = 0; i < 256; i++){
		if(abs(dis.x) < 0.01 || f > maxdepth){
			break;
		}

		f += dis.x;
		point = cam + screenp * f;
		dis = distanceToObj(p);
	}

	if(f < maxdepth){
		if(dis.y == 0.0){
			result = vec3(0.5, 0.5, 0.5);
		}else{
			result = vec3(5.9, 0.0, 0.0);
		}

		vec3 normal = normalize(vec3(dis.x - distanceToObj(point - e.xyy).x, 
			dis.x - distanceToObj(point - e.yxy).x,
			dis.x - distanceToObj(point - e.yyx).x));

		float b = dot(normal, normalize(cam - point));

		result = (b * result + pow(b, 16.0)) * (1.0 - f * 0.01);
	}else{
		result = vec3(0.0, 0.0, 0.0);
	}

	result.r = 0.5 + sin(time);
	
	color = vec4(result, 1.0) + prev;
}
