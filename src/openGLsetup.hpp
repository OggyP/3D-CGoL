#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

GLuint createShader(GLenum type, const GLchar* src);
GLuint createShader(GLenum type, const GLchar* src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	return shader;
}

void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& shaderProgram);
void createShaderProgram(const GLchar* vertSrc, const GLchar* fragSrc, GLuint& vertexShader, GLuint& fragmentShader, GLuint& shaderProgram)
{
	// Create and compile the vertex shader
	vertexShader = createShader(GL_VERTEX_SHADER, vertSrc);

	// Create and compile the fragment shader
	fragmentShader = createShader(GL_FRAGMENT_SHADER, fragSrc);

	// Link the vertex and fragment shader into a shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);

	char buffer[512];
	glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
	glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
	std::cout << buffer << std::endl;
}

void specifySceneVertexAttributes(GLuint shaderProgram);
void specifySceneVertexAttributes(GLuint shaderProgram)
{
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint brightAttrib = glGetAttribLocation(shaderProgram, "brightness");
	glEnableVertexAttribArray(brightAttrib);
	glVertexAttribPointer(brightAttrib, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
}

void specifyScreenVertexAttributes(GLuint shaderProgram);
void specifyScreenVertexAttributes(GLuint shaderProgram)
{
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
}