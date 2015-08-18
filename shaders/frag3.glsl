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

uniform sampler2D tex;

vec2 distanceToObj(in vec3 point)
{
	return vec2(point.y + 3.0, 0.0);
}

void main()
{
	vec4 prev = texture(tex, texCoord / transition) * transition;

	// Camera
	vec2 texscreen = texCoord * 2.0 - 1.0;

	const vec3 up = vec3(0.0, 1.0, 0.0);
	const vec3 lookat = vec3(0.0, 0.0, 0.0);
	vec3 cam = vec3(-sin(time / 1000.0) * 8.0, 4.0, cos(time / 100.0) * 8.0);

	vec3 p = normalize(lookat - cam);
	vec3 u = normalize(cross(up, p));
	vec3 v = cross(p, u);
	vec3 cv = cam + p;
	vec3 screen = cv + texscreen.x * u * (800.0 / 600.0) + texscreen.y * v;
	vec3 screenp = normalize(screen - cam);

	const vec3 e = vec3(0.1, 0.0, 0.0);
	const float maxdepth = 60.0;
	vec2 dis = vec2(0.1, 0.0);
	
	vec3 result, point;
	float f = 1.0;
	int i = 0;
	while(true){
		if(abs(dis.x) < 0.01 || f > maxdepth || i >= 10){
			break;
		}

		f += dis.x;
		point = cam + screenp * f;
		dis = distanceToObj(p);

		i++;
	}

	if(f < maxdepth){
		if(dis.y == 0.0){
			result = vec3(0.5, 0.5, 0.5);
		}else{
			result = vec3(0.0, 0.0, 0.0);
		}

		vec3 normal = normalize(vec3(dis.x - distanceToObj(point - e.xyy).x, 
			dis.x - distanceToObj(point - e.yxy).x,
			dis.x - distanceToObj(point - e.yyx).x));

		float b = dot(normal, normalize(cam - point));

		result = (b * result + pow(b, 8.0)) * (1.0 - f * 0.01);
	}else{
		result = vec3(0.0, 0.0, 0.0);
	}
	
	color = vec4(result, 1.0) + prev;
}
