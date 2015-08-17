#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "shader.h"
#include "utils.h"

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

GLuint vao, vbo;
int time;

GLuint proga, prog1, prog2;
GLint peakloca, peakloc1, peakloc2;

void initVis()
{
	glewInit();

#ifdef VISMU_DEBUG
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
#endif
	prog1 = loadProgramFile("shaders/frag1.glsl", "shaders/vert.glsl");
	peakloc1 = glGetUniformLocation(prog1, "peak");

	prog2 = loadProgramFile("shaders/frag2.glsl", "shaders/vert.glsl");
	peakloc2 = glGetUniformLocation(prog2, "peak");

	loadScreenTriangles(&vao, &vbo);
	
	glUseProgram(prog1);
	proga = prog1;
	peakloca = peakloc1;

	time = 0;
}

void renderVis(double peak)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glProgramUniform1f(proga, peakloca, peak);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	time++;

	if(time > 100){
		if(proga == prog1){
			proga = prog2;
			peakloca = peakloc2;
		}else{
			proga = prog1;
			peakloca = peakloc1;
		}
		glUseProgram(proga);

		time = 0;
	}
}
