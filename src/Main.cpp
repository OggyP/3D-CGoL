#include "Platform/Platform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

#include "Vectors.hpp"
#include "openGLsetup.hpp"
#include "verticies.hpp"

#define ARRAY_SIZE 100
#define RAND_CHANCE 3

bool running = true;

Vector3f position;
Vector3f lookingAt;

const int rules[2][2] = {
	{ 5, 6 }, // Alive between
	{ 5, 5 }  // Dead between
};

sf::Mutex CGoLMutex;
bool verticiesUpdate = false;
bool CGoLArray[ARRAY_SIZE][ARRAY_SIZE][ARRAY_SIZE] = { 0 };
std::vector<GLfloat> verticies;

int lookingAtBlock[3] = { 0 };

bool paused = true;
bool showCursor = true;
bool clearBoard = false;

sf::Mutex editMutex;
std::vector<std::pair<std::array<int, 3>, bool>> edits; // second is true add else delete

// CGoL
const GLchar* CGOLVertexSource = R"glsl(
    #version 150 core

    in vec3 position;
	in float brightness;

	out float Brightness;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;

    void main()
    {
		vec4 newPos = vec4(position, 1.0);
        gl_Position =  proj * view * model * newPos;
		Brightness = brightness;
    }
)glsl";

const GLchar* CGOLFragmentSource = R"glsl(
	#version 150 core
	in float Brightness;

	out vec4 outColor;

	void main()
	{
		outColor = vec4(Brightness, Brightness, Brightness, 1.0);
	}
)glsl";

// Cube Looking At Block
const GLchar* blockOutlineVertexSource = R"glsl(
    #version 150 core

    in vec3 position;

	uniform vec3 drawPos;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;

    void main()
    {
		vec4 newPos = vec4(position + drawPos, 1.0);
        gl_Position =  proj * view * model * newPos;
    }
)glsl";

const GLchar* blockOutlineFragmentSource = R"glsl(
	#version 150 core

	out vec4 outColor;

	void main()
	{
		outColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
)glsl";

// Screen
const GLchar* screenVertexSource = R"glsl(
    #version 150 core
    in vec2 position;
    in vec2 texcoord;
    out vec2 Texcoord;
    void main()
    {
        Texcoord = texcoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";
const GLchar* screenFragmentSource = R"glsl(
    #version 150 core
    in vec2 Texcoord;

    out vec4 outColor;

    uniform sampler2D texImage;

    void main()
	{
		outColor = texture(texImage, Texcoord);
    }
)glsl";

void renderingThread(sf::Window* window);
void renderingThread(sf::Window* window)
{
	// activate the window's context
	window->setActive(true);

	position.z = 5;

	glewExperimental = GL_TRUE;
	glewInit();

	// VAO
	GLuint vaoQuad, vaoCube, vaoBoard;
	glGenVertexArrays(1, &vaoQuad);
	glGenVertexArrays(1, &vaoCube);
	glGenVertexArrays(1, &vaoBoard);

	// VBO
	GLuint vboQuad, vboCube, vboBoard;
	glGenBuffers(1, &vboBoard);
	glGenBuffers(1, &vboCube);
	glGenBuffers(1, &vboQuad);

	// Bind Verticies
	float screenSize[2] = { (float)window->getSize().x, (float)window->getSize().y };

	std::cout << screenSize[0] << '|' << screenSize[1];

	glBindVertexArray(vaoBoard);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoard);
	// glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(boardVertices), boardVertices, GL_STATIC_DRAW);

	glBindVertexArray(vaoQuad);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glBindVertexArray(vaoCube);
	glBindBuffer(GL_ARRAY_BUFFER, vboCube);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	// Create Cube Shader Program
	GLuint CubeVertexShader, CubeFragmentShader, cubeShaderProgram;
	createShaderProgram(blockOutlineVertexSource, blockOutlineFragmentSource, CubeVertexShader, CubeFragmentShader, cubeShaderProgram);

	specifyOutlineVertexAttributes(cubeShaderProgram);

	// Create CGoL Shader Program
	GLuint CGOLVertexShader, CGOLFragmentShader, CGOLShaderProgram;
	createShaderProgram(CGOLVertexSource, CGOLFragmentSource, CGOLVertexShader, CGOLFragmentShader, CGOLShaderProgram);

	glBindVertexArray(vaoBoard);
	glBindBuffer(GL_ARRAY_BUFFER, vboBoard);

	specifySceneVertexAttributes(CGOLShaderProgram);

	// Create framebuffer ==============================================================================================
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create Screen Shader Program and Frame Buffer
	GLuint screenVertexShader, screenFragmentShader, screenShaderProgram;
	createShaderProgram(screenVertexSource, screenFragmentSource, screenVertexShader, screenFragmentShader, screenShaderProgram);

	glBindVertexArray(vaoQuad);
	glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
	specifyScreenVertexAttributes(screenShaderProgram);

	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// Create texture to hold color buffer
	GLuint texColorBuffer;
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window->getSize().x, window->getSize().y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

	// Create Renderbuffer Object to hold depth and stencil buffers
	GLuint rboDepthStencil;
	glGenRenderbuffers(1, &rboDepthStencil);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenSize[0], screenSize[1]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthStencil);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//==================================================================================================================

	glm::mat4 proj = glm::perspective(glm::radians(90.0f), (float)screenSize[0] / screenSize[1], 0.001f, 5600.0f);
	glm::mat4 model = glm::mat4(0.1f);

	glUseProgram(cubeShaderProgram);

	GLint uniDrawPos = glGetUniformLocation(cubeShaderProgram, "drawPos");

	GLint uniViewCube = glGetUniformLocation(cubeShaderProgram, "view");

	GLint uniProjCube = glGetUniformLocation(cubeShaderProgram, "proj");
	glUniformMatrix4fv(uniProjCube, 1, GL_FALSE, glm::value_ptr(proj));

	GLint uniModelCube = glGetUniformLocation(cubeShaderProgram, "model");
	glUniformMatrix4fv(uniModelCube, 1, GL_FALSE, glm::value_ptr(model));

	// Use CGOL Shader and setup uniforms
	glUseProgram(CGOLShaderProgram);

	GLint uniViewCGoL = glGetUniformLocation(CGOLShaderProgram, "view");

	GLint uniProjCGoL = glGetUniformLocation(CGOLShaderProgram, "proj");
	glUniformMatrix4fv(uniProjCGoL, 1, GL_FALSE, glm::value_ptr(proj));

	GLint uniModelCGoL = glGetUniformLocation(CGOLShaderProgram, "model");
	glUniformMatrix4fv(uniModelCGoL, 1, GL_FALSE, glm::value_ptr(model));

	// enable depth testing (2D)
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);

	int drawSize = 0;

	lookingAt.x = ARRAY_SIZE / 2;
	lookingAt.y = ARRAY_SIZE / 2;
	lookingAt.z = ARRAY_SIZE / 2;

	while (running)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glClearColor(0.0, 0.0, 0.0, 1.0f);
		// Clear the screen to white
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(CGOLShaderProgram);

		glBindVertexArray(vaoBoard);
		glBindBuffer(GL_ARRAY_BUFFER, vboBoard);

		glEnable(GL_DEPTH_TEST);

		glm::mat4 view = glm::lookAt(
			glm::vec3(position.x, position.y, position.z),
			glm::vec3(lookingAt.x + position.x, lookingAt.y + position.y, lookingAt.z + position.z),
			glm::vec3(0.0f, 0.0f, 1.0f));

		glUniformMatrix4fv(uniViewCGoL, 1, GL_FALSE, glm::value_ptr(view));

		CGoLMutex.lock();
		if (verticiesUpdate)
		{
			verticiesUpdate = false;
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verticies.size(), verticies.data(), GL_DYNAMIC_DRAW);
			drawSize = verticies.size() / 4;
		}
		CGoLMutex.unlock();

		// std::cout << drawSize << " size\n";

		glDrawArrays(GL_TRIANGLES, 0, drawSize);

		if (showCursor)
		{
			bool vaildPosition = true;
			for (int i = 0; i < 3; i++)
				if (lookingAtBlock[i] >= ARRAY_SIZE || lookingAtBlock[i] < 0)
					vaildPosition = false;
			if (vaildPosition)
			{
				glUseProgram(cubeShaderProgram);
				glBindVertexArray(vaoCube);
				glBindBuffer(GL_ARRAY_BUFFER, vboCube);

				glUniform3f(uniDrawPos, lookingAtBlock[0], lookingAtBlock[1], lookingAtBlock[2]);
				glUniformMatrix4fv(uniViewCube, 1, GL_FALSE, glm::value_ptr(view));

				glDrawArrays(GL_LINES, 0, 24);
			}
		}

		glDisable(GL_DEPTH_TEST);

		// =============================================================

		glUseProgram(screenShaderProgram);
		// Bind to actual screen (frame buffer 0)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texColorBuffer);

		// Bind correct stuff
		glBindVertexArray(vaoQuad);
		glBindBuffer(GL_ARRAY_BUFFER, vboQuad);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		window->display();
	}

	glDeleteFramebuffers(1, &frameBuffer);

	glDeleteProgram(CGOLShaderProgram);
	glDeleteShader(CGOLFragmentShader);
	glDeleteShader(CGOLVertexShader);

	glDeleteBuffers(1, &vboBoard);
	glDeleteVertexArrays(1, &vboBoard);

	glDeleteProgram(screenShaderProgram);
	glDeleteShader(screenFragmentShader);
	glDeleteShader(screenVertexShader);

	glDeleteBuffers(1, &vboQuad);
	glDeleteVertexArrays(1, &vboQuad);
}

bool pointIsAlive(int x, int y, int z);
bool pointIsAlive(int x, int y, int z)
{
	return CGoLArray[(x >= ARRAY_SIZE ? 0 : x < 0 ? ARRAY_SIZE - 1 :
													  x)][(y >= ARRAY_SIZE ? 0 : y < 0 ? ARRAY_SIZE - 1 :
																						 y)][(z >= ARRAY_SIZE ? 0 : z < 0 ? ARRAY_SIZE - 1 :
																															z)];
}

std::vector<GLfloat> getVerticies(float x, float y, float z, float brightness);
std::vector<GLfloat> getVerticies(float x, float y, float z, float brightness)
{
	std::vector<GLfloat> verticies = {
		-0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness,
		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness,
		0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness,
		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness,
		-0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness,
		-0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness,

		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.7f,
		0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.7f,
		0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.7f,
		0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.7f,
		-0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.7f,
		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.7f,

		-0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.75f,
		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		-0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		-0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.75f,
		-0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness * 0.75f,

		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.75f,
		0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.75f,
		0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness * 0.75f,
		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.75f,

		-0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.86f,
		-0.5f + x,
		0.5f + y,
		0.5f + z,
		brightness * 0.86f,
		-0.5f + x,
		0.5f + y,
		-0.5f + z,
		brightness * 0.86f,

		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.86f,
		0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness * 0.86f,
		-0.5f + x,
		-0.5f + y,
		-0.5f + z,
		brightness * 0.86f,
		-0.5f + x,
		-0.5f + y,
		0.5f + z,
		brightness * 0.86f
	};
	return verticies;
}

void arrayUpdateThread();
void arrayUpdateThread()
{
	srand(time(0));
	for (int x = 0; x < ARRAY_SIZE; x++)
		for (int y = 0; y < ARRAY_SIZE; y++)
			for (int z = 0; z < ARRAY_SIZE; z++)
			{
				if (rand() % RAND_CHANCE == 0)
					CGoLArray[x][y][z] = 1;
				else
					CGoLArray[x][y][z] = 0;
			}
	bool firstRun = true;
	while (running)
	{

		// sf::sleep(sf::milliseconds(200));
		editMutex.lock();
		for (auto const& updateBlock : edits)
		{
			std::cout << updateBlock.second << "\n";
			CGoLArray[updateBlock.first[0]][updateBlock.first[1]][updateBlock.first[2]] = updateBlock.second;
			CGoLMutex.lock();
			std::vector<GLfloat> cube = getVerticies(updateBlock.first[0], updateBlock.first[1], updateBlock.first[2], 1.0f);
			verticies.insert(verticies.end(), cube.begin(), cube.end());
			verticiesUpdate = true;
			CGoLMutex.unlock();
		}
		edits.clear();
		editMutex.unlock();
		if (clearBoard)
		{
			clearBoard = false;
			bool newCGOLArray[ARRAY_SIZE][ARRAY_SIZE][ARRAY_SIZE];
			for (int x = 0; x < ARRAY_SIZE; x++)
				for (int y = 0; y < ARRAY_SIZE; y++)
					for (int z = 0; z < ARRAY_SIZE; z++)
					{
						newCGOLArray[x][y][z] = 0;
					}
			CGoLMutex.lock();
			verticiesUpdate = true;
			verticies.clear();
			memcpy(CGoLArray, newCGOLArray, sizeof(CGoLArray));
			CGoLMutex.unlock();
			CGoLMutex.unlock();
		}
		if (!paused || firstRun)
		{
			bool newCGOLArray[ARRAY_SIZE][ARRAY_SIZE][ARRAY_SIZE];
			std::vector<GLfloat> newVerticies;
			for (int x = 0; x < ARRAY_SIZE; x++)
				for (int y = 0; y < ARRAY_SIZE; y++)
					for (int z = 0; z < ARRAY_SIZE; z++)
					{
						// std::cout << "X " << x << " | Y " << y << " | Z " << z << "\n";
						int neighbours = 0;
						for (int xOffset = -1; xOffset < 2; xOffset++)
							for (int yOffset = -1; yOffset < 2; yOffset++)
								for (int zOffset = -1; zOffset < 2; zOffset++)
									if (xOffset != 0 || yOffset != 0 || zOffset != 0)
									{
										if (pointIsAlive(x + xOffset, y + yOffset, z + zOffset))
											neighbours++;
									}
						// if (neighbours > 0)
						// {
						// 	std::cout << "Almost ALive " << neighbours << "\n";
						// }
						if (neighbours > 0)
						{
							if (CGoLArray[x][y][z])
							{
								if (neighbours >= rules[0][0] && neighbours <= rules[0][1])
								{
									// Alive
									newCGOLArray[x][y][z] = 1;
									std::vector<GLfloat> cube = getVerticies(x, y, z, 1.0f);
									newVerticies.insert(newVerticies.end(), cube.begin(), cube.end());
								}
								else
									newCGOLArray[x][y][z] = 0;
							}
							else
							{
								if (neighbours >= rules[1][0] && neighbours <= rules[1][1])
								{
									// Alive
									newCGOLArray[x][y][z] = 1;
									std::vector<GLfloat> cube = getVerticies(x, y, z, 1.0f);
									newVerticies.insert(newVerticies.end(), cube.begin(), cube.end());
								}
								else
									newCGOLArray[x][y][z] = 0;
							}
						}
					}
			CGoLMutex.lock();
			verticiesUpdate = true;
			verticies = newVerticies;
			memcpy(CGoLArray, newCGOLArray, sizeof(CGoLArray));
			CGoLMutex.unlock();
			firstRun = false;
		}
		else
		{
			sf::sleep(sf::microseconds(5));
		}
	}
}

int main()
{
	util::Platform platform;

#if defined(_DEBUG)
	std::cout << "Hello World!" << std::endl;
#endif
	// Open GL Settings
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 2; // Optional
	// Request OpenGL version 3.2
	settings.majorVersion = 3;
	settings.minorVersion = 2;
	settings.attributeFlags = sf::ContextSettings::Core;

#if defined(_DEBUG)
	sf::Window window(sf::VideoMode(1000, 1000), "OpenGL 3D Conway's Game of Life DEBUGGING MODE", sf::Style::Resize | sf::Style::Close, settings);
#else
	#if defined(__linux__)
	sf::Window window(sf::VideoMode(1000, 1000), "OpenGL 3D Conway's Game of Life", sf::Style::Resize | sf::Style::Fullscreen | sf::Style::Close, settings);
	#else
	sf::Window window(sf::VideoMode(1000, 1000), "OpenGL 3D Conway's Game of Life", sf::Style::Resize | sf::Style::Close, settings);
	platform.toggleFullscreen(window.getSystemHandle(), sf::Style::Fullscreen, false, sf::Vector2u(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height));
	#endif
#endif

	platform.setIcon(window.getSystemHandle());
	window.setFramerateLimit(60);
	window.setActive(false);

	// launch the rendering thread
	sf::Thread renderThread(&renderingThread, &window);
	renderThread.launch();

	sf::Thread arrayUpdatingThread(&arrayUpdateThread);
	arrayUpdatingThread.launch();

	sf::Event event;
	sf::Clock deltaClock;

	const float movementSpeed = 0.2f;
	const float mouseSensitivity = 0.0015;
	auto screenSize = window.getSize();
	sf::Vector2i mouseCoord;

	while (running)
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
				running = false;
			}
		}

		float toMoveSpeed = (sf::Mouse::isButtonPressed(sf::Mouse::Middle)) ? movementSpeed / 6 : movementSpeed;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
		{
			window.close();
			running = false;
		}

		sf::Time dt = deltaClock.restart();

		float deltaTimeMovementSpeed = toMoveSpeed * (float)dt.asMicroseconds() / 10000.0f;

		lookingAt.updateDirection();

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
		{
			CGoLMutex.lock();
			for (int x = 0; x < ARRAY_SIZE; x++)
				for (int y = 0; y < ARRAY_SIZE; y++)
					for (int z = 0; z < ARRAY_SIZE; z++)
					{
						if (rand() % RAND_CHANCE == 0)
							CGoLArray[x][y][z] = 1;
						else
							CGoLArray[x][y][z] = 0;
					}
			CGoLMutex.unlock();
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::P))
		{
			paused = !paused;
			while (sf::Keyboard::isKeyPressed(sf::Keyboard::P))
			{
				sf::sleep(sf::microseconds(5));
				deltaClock.restart();
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
		{
			showCursor = !showCursor;
			while (sf::Keyboard::isKeyPressed(sf::Keyboard::L))
			{
				sf::sleep(sf::microseconds(5));
				deltaClock.restart();
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
		{
			clearBoard = true;
		}

		mouseCoord = sf::Mouse::getPosition(window);

		if (mouseCoord.x != (int)screenSize.x / 2 || mouseCoord.y != (int)screenSize.y / 2)
		{
			// Mouse has moved
			lookingAt.directionH -= (mouseCoord.x - (int)screenSize.x / 2) * mouseSensitivity;
			lookingAt.directionV -= (mouseCoord.y - (int)screenSize.y / 2) * mouseSensitivity;
			if (lookingAt.directionV > M_PI_2 - 0.1)
			{
				lookingAt.directionV = M_PI_2 - 0.1;
			}
			if (lookingAt.directionV < -M_PI_2 + 0.1)
			{
				lookingAt.directionV = -M_PI_2 + 0.1;
			}

			lookingAt.updateCoords(10);

			// Move mouse back to the middle of the screen
			sf::Mouse::setPosition(sf::Vector2i(screenSize.x / 2, screenSize.y / 2), window);
		}

		Vector3f moveAmt;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			moveAmt.x += cos(lookingAt.directionH) * deltaTimeMovementSpeed;
			moveAmt.y += sin(lookingAt.directionH) * deltaTimeMovementSpeed;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			moveAmt.x += -cos(lookingAt.directionH) * deltaTimeMovementSpeed;
			moveAmt.y += -sin(lookingAt.directionH) * deltaTimeMovementSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			moveAmt.x += cos(lookingAt.directionH + M_PI_2) * deltaTimeMovementSpeed;
			moveAmt.y += sin(lookingAt.directionH + M_PI_2) * deltaTimeMovementSpeed;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			moveAmt.x += -cos(lookingAt.directionH + M_PI_2) * deltaTimeMovementSpeed;
			moveAmt.y += -sin(lookingAt.directionH + M_PI_2) * deltaTimeMovementSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
		{
			moveAmt.z += deltaTimeMovementSpeed;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
		{
			moveAmt.z -= deltaTimeMovementSpeed;
		}

		position.x += moveAmt.x;
		position.y += moveAmt.y;
		position.z += moveAmt.z;
		lookingAt.updateDirection();

		lookingAt.normalise(5);

		lookingAtBlock[0] = (int)floor(position.x + lookingAt.x + 0.5f);
		lookingAtBlock[1] = (int)floor(position.y + lookingAt.y + 0.5f);
		lookingAtBlock[2] = (int)floor(position.z + lookingAt.z + 0.5f);

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right))
		{
			std::array<int, 3> currentBlock = { (int)floor(lookingAtBlock[0]), (int)floor(lookingAtBlock[1]), (int)floor(lookingAtBlock[2]) };
			bool vaildPosition = true;
			for (int i = 0; i < 3; i++)
				if (currentBlock[i] >= ARRAY_SIZE || currentBlock[i] < 0)
					vaildPosition = false;
			if (vaildPosition)
			{
				editMutex.lock();
				edits.push_back(std::make_pair(currentBlock, sf::Mouse::isButtonPressed(sf::Mouse::Left)));
				editMutex.unlock();
			}
			while (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right))
			{
				sf::sleep(sf::microseconds(2));
				deltaClock.restart();
			}
		}

		// sf::sleep(sf::microseconds(5));
	}

	renderThread.wait();
	arrayUpdatingThread.wait();
	return 0;
}