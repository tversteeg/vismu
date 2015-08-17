#include <GL/glew.h>

GLuint loadShader(char *source, GLenum type);

GLuint loadProgram(char *fragmentsrc, char *vertexsrc);

void loadScreenTriangles(GLuint *vao, GLuint *vbo);

void initVis();
void renderVis(double peak);
