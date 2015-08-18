#include <GL/glew.h>

GLuint loadShader(char *source, GLenum type);
GLuint loadProgram(char *fragmentsrc, char *vertexsrc);
GLuint loadProgramFile(const char *fragment, const char *vertex);
void loadScreenTriangles(GLuint *vao, GLuint *vbo);

void initVis(int width, int height);
void renderVis(double peak);
