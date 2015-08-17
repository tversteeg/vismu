#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shader.h"
#include "utils.h"

#define SHADERS 2
#define SHADER_TIME 400
#define TRANSITION_TIME 100.0

typedef struct {
	GLuint program;
	GLint peak, transition, texture, time;
} visprog_t;

GLuint loadShader(char *source, GLenum type)
{
	GLuint shader;
	GLint status, loglen;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char**)&source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE){
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);

		char *buf = (char*)malloc(loglen + 1);
		glGetShaderInfoLog(shader, loglen, NULL, buf);

		printf("%s shader error:\n\t%s", (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment", buf);

		free(buf);	
	}

	return shader;
}

GLuint loadProgram(char *fragmentsrc, char *vertexsrc)
{
	GLuint program, vertex, fragment;

	fragment = loadShader(fragmentsrc, GL_FRAGMENT_SHADER);
	vertex = loadShader(vertexsrc, GL_VERTEX_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	glBindAttribLocation(program, 0, "position");
	glLinkProgram(program);

	return program;
}

GLuint loadProgramFile(const char *fragment, const char *vertex)
{
	GLuint program;

	char *fragsrc, *vertsrc;
	if(loadTextFile(fragment, &fragsrc) < 0){
		fprintf(stderr, "Could not load shader file: %s\n", fragment);
		exit(1);
	}
	if(loadTextFile(vertex, &vertsrc) < 0){
		fprintf(stderr, "Could not load shader file: %s\n", vertex);
		exit(1);
	}

	program = loadProgram(fragsrc, vertsrc);

	free(fragsrc);
	free(vertsrc);

	return program;
}

void loadScreenTriangles(GLuint *vao, GLuint *vbo)
{
	GLfloat vertices[] = {
		-1,-1, 0,
		1,-1, 0,
		-1, 1, 0,
		1,-1, 0,
		-1, 1, 0,
		1, 1, 0,
	};

	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

GLuint vao, vbo, tex;
int time, active;

visprog_t progs[SHADERS];

void initVis()
{
	glewInit();

#ifdef VISMU_DEBUG
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
#endif

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	int i;
	for(i = 0; i < SHADERS; i++){
		char fragshadername[64];
		snprintf(fragshadername, sizeof(fragshadername), "shaders/frag%d.glsl", i + 1);
		progs[i].program = loadProgramFile(fragshadername, "shaders/vert.glsl");

		progs[i].peak = glGetUniformLocation(progs[i].program, "peak");
		progs[i].transition = glGetUniformLocation(progs[i].program, "transition");
		progs[i].time = glGetUniformLocation(progs[i].program, "time");
		progs[i].texture = glGetUniformLocation(progs[i].program, "tex");
	}

	loadScreenTriangles(&vao, &vbo);
	
	active = 1;
	glUseProgram(progs[active].program);

	time = 0;
}

void renderVis(double peak)
{
	double transition = 0.0;
	time++;
	if(time > SHADER_TIME){
		active++;
		if(active == SHADERS){
			active = 0;
		}

		time = 0;
	}

	if(time <= TRANSITION_TIME){
		transition = 1.0 - time / TRANSITION_TIME;

		int prev = active > 0 ? active - 1 : SHADERS - 1;
		
		glUseProgram(progs[prev].program);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glProgramUniform1f(progs[prev].program, progs[prev].peak, peak);
		glProgramUniform1f(progs[prev].program, progs[prev].transition, 0);
		glProgramUniform1f(progs[prev].program, progs[prev].time, time);
		glProgramUniform1i(progs[prev].program, progs[prev].texture, 0);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 800, 600, 0);

		glUseProgram(progs[active].program);
		glProgramUniform1i(progs[active].program, progs[active].texture, 0);
	}

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glProgramUniform1f(progs[active].program, progs[active].peak, peak);
	glProgramUniform1f(progs[active].program, progs[active].transition, transition);
	glProgramUniform1f(progs[active].program, progs[active].time, time);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}
