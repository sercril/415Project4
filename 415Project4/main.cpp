#define USE_PRIMITIVE_RESTART
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <fstream>
#include <stack>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>

#include "LoadShaders.h"
#include "SceneObject.h"

#include "Texture.h"

#pragma comment (lib, "glew32.lib")
#pragma warning (disable : 4996) // Windows ; consider instead replacing fopen with fopen_s

using namespace std;

#pragma region Structs and Enums

enum ObjectType
{
	FLOOR = 0,
	BALL
};

struct SceneNode
{	
	SceneObject object;
	ObjectType type;
	SceneNode* parent;
	std::vector<SceneNode *> children;
	int id;

	SceneNode()
	{

	}
};



struct Keyframe
{
	unsigned long time; // Timestamp, milliseconds since first record. Assume nondecreasing order.
	float palm_p[3];    // palm position w.r.t. world (x, y, z)
	float palm_q[4];    // palm orientation w.r.t. world, quaternion (a, b, c, s) a.k.a. (x, y, z, w)
	float joint[16];    // finger joint angles (in radians). See list above.
	float ball_p[3];    // ball position
	float ball_q[4];    // ball orientation
};


#pragma endregion

#pragma region "Global Variables"

#define SCREEN_WIDTH 1920.0f
#define SCREEN_HEIGHT 1080.0f
#define NUM_OBJECTS 2
#define INDECIES 10000

int mouseX, mouseY,
mouseDeltaX, mouseDeltaY;

unsigned int textureWidth, textureHeight;
unsigned char *imageData;

bool genNorms;

float azimuth, elevation, ballRadius, floorY, cameraZFactor,
		nearValue, farValue, leftValue, rightValue, topValue, bottomValue;

struct Keyframe c;

GLuint Matrix_loc, vertposition_loc, vertcolor_loc, normal_loc, vertex_UV, texture_location;

GLenum errCode;

const GLubyte *errString;


gmtl::Matrix44f view, modelView, viewScale, camera, projection, normalMatrix,
				elevationRotation, azimuthRotation, cameraZ;


std::vector<SceneNode*> sceneGraph;
std::vector<GLfloat> ball_vertex_data, ball_normal_data, ball_uv_data;
std::vector<GLushort> ball_index_data;
std::vector<Keyframe> keyframes;

std::vector<Vertex> ballData;

#pragma endregion

#pragma region Camera

float arcToDegrees(float arcLength)
{
	return ((arcLength * 360.0f) / (2.0f * M_PI));
}

float degreesToRadians(float deg)
{
	return (2.0f * M_PI *(deg / 360.0f));
}

void cameraRotate()
{

	elevationRotation.set(
		1, 0, 0, 0,
		0, cos(elevation * -1), (sin(elevation * -1) * -1), 0,
		0, sin(elevation * -1), cos(elevation * -1), 0,
		0, 0, 0, 1);

	azimuthRotation.set(
		cos(azimuth * -1), 0, sin(azimuth * -1), 0,
		0, 1, 0, 0,
		(sin(azimuth * -1) * -1), 0, cos(azimuth * -1), 0,
		0, 0, 0, 1);

	elevationRotation.setState(gmtl::Matrix44f::ORTHOGONAL);

	azimuthRotation.setState(gmtl::Matrix44f::ORTHOGONAL);

	view = azimuthRotation * elevationRotation * cameraZ;

	gmtl::invert(view);

	glutPostRedisplay();
}

#pragma endregion

#pragma region Helper Functions


void LoadTexture(char* filename)
{
	LoadPPM(filename, &textureWidth, &textureHeight, &imageData, 1);
}

void importBallData()
{
	ifstream fp;
	int i = 0, j = 0, k=0, numVerticies, numIndicies, numPolygons;

	fp.open("SpherePNT.txt", ios_base::in);

	if (fp)
	{

		for (std::string line; std::getline(fp, line); ++i)
		{
			std::istringstream in(line);

			if (i == 0)
			{
				in >> numVerticies;
				ball_vertex_data.resize(numVerticies*3);
				ball_normal_data.resize(numVerticies * 3);
				ball_uv_data.resize(numVerticies * 2);
			}
			else if (i > 0 && i <= numVerticies)
			{
				in >> ball_vertex_data[j] >> ball_vertex_data[j + 1] >> ball_vertex_data[j + 2] >> ball_normal_data[j] >> ball_normal_data[j + 1] >> ball_normal_data[j + 2] >> ball_uv_data[k] >> ball_uv_data[k+1];
				j += 3;
				k += 2;
			}
			else if (i == (numVerticies + 1))
			{
				in >> numPolygons;
				ball_index_data.resize(numPolygons*3);
				j = 0;
			}
			else if (i > (numVerticies + 1))
			{
				in >> numIndicies >> ball_index_data[j] >> ball_index_data[j + 1] >> ball_index_data[j + 2];
				j+=3;
			}
		}

		fp.close();
	}
}


void buildGraph()
{
	
	SceneNode* ball = new SceneNode();
	SceneNode* floor = new SceneNode();
	gmtl::Matrix44f initialTranslation, moveLeft;
		
	//Ball
	importBallData();
	ball->type = BALL;
	ball->parent = NULL;
	ball->object = SceneObject(ballRadius, ball_vertex_data, ball_normal_data, ball_uv_data, ball_index_data, vertposition_loc, vertex_UV);
	ball->children.clear();
	LoadTexture("marbles2.ppm");
	ball->object.SetTexture(Texture(textureWidth, textureHeight, imageData));
	sceneGraph.push_back(ball);

	//Floor
	floor->type = FLOOR;
	floor->parent = NULL;
	floor->object = SceneObject(ballRadius * 10, 1.0f, ballRadius * 10, vertposition_loc, vertex_UV);
	floor->children.clear();
	initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f, floorY*-1.0f, 0.0f));
	initialTranslation.setState(gmtl::Matrix44f::TRANS);
	//Make it look good
	initialTranslation = initialTranslation;
	floor->object.matrix = floor->object.matrix * initialTranslation;
	floor->object.SetTexture(Texture(textureWidth, textureHeight, imageData));
	sceneGraph.push_back(floor);
}

void renderGraph(std::vector<SceneNode*> graph, gmtl::Matrix44f mv)
{
	gmtl::Matrix44f thisTransform, renderTransform, inverseTransform;

	gmtl::identity(thisTransform);

	if(!graph.empty())
	{
		for (int i = 0; i < graph.size(); ++i)
		{
			gmtl::Matrix44f newMV = mv * graph[i]->object.matrix;

			newMV = newMV * graph[i]->object.scale;


			if (genNorms)
			{
				graph[i]->object.VAO.GenerateNormals();
			}

			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, graph[i]->object.texture.textureHeight, graph[i]->object.texture.textureWidth, 0, GL_RGB, GL_UNSIGNED_BYTE, graph[i]->object.texture.imageData);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			

			

			//Render
			renderTransform = projection * newMV;
			
			glBindVertexArray(graph[i]->object.VAO.vertexArray);
			// Send a different transformation matrix to the shader
			glUniformMatrix4fv(Matrix_loc, 1, GL_FALSE, &renderTransform[0][0]);

			// Draw the transformed cuboid
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFF);
			glDrawElements(GL_TRIANGLES, INDECIES, GL_UNSIGNED_SHORT, NULL);
			
			if (!graph[i]->children.empty())
			{
				renderGraph(graph[i]->children, newMV);
			}

		}
	}
	
	return;
}

#pragma endregion

#pragma region "Input"

# pragma region "Mouse Input"

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON)
	{
		mouseX = x;
		mouseY = y;
	}
}

void mouseMotion(int x, int y)
{

	mouseDeltaX = x - mouseX;
	mouseDeltaY = y - mouseY;


	elevation += degreesToRadians(arcToDegrees(mouseDeltaY)) / (SCREEN_HEIGHT/2);
	azimuth += degreesToRadians(arcToDegrees(mouseDeltaX)) / (SCREEN_WIDTH /2 );

	cameraRotate();

	mouseX = x;
	mouseY = y;

}

# pragma endregion

#pragma region "Keyboard Input"

void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{

		case 'n':
			genNorms = true;
			break;

		case 'Z':
			cameraZFactor += 10.f;
			cameraZ = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f, 0.0f, cameraZFactor));
			cameraZ.setState(gmtl::Matrix44f::TRANS);
			view = azimuthRotation * elevationRotation * cameraZ;
			gmtl::invert(view);			
			break;

		case 'z':
			cameraZFactor -= 10.f;
			cameraZ = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f, 0.0f, cameraZFactor));
			cameraZ.setState(gmtl::Matrix44f::TRANS);
			view = azimuthRotation * elevationRotation * cameraZ;
			gmtl::invert(view);
			break;

		case 'q': case 'Q': case 033 /* Escape key */:
			exit(EXIT_SUCCESS);
			break;
	}
	
	glutPostRedisplay();
}

#pragma endregion

#pragma endregion

#pragma region "GLUT Functions"

void display()
{

	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	renderGraph(sceneGraph, view);
	genNorms = false;
	//Ask GL to execute the commands from the buffer
	glutSwapBuffers();	// *** if you are using GLUT_DOUBLE, use glutSwapBuffers() instead 

	//Check for any errors and print the error string.
	//NOTE: The string returned by gluErrorString must not be altered.
	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		cout << "OpengGL Error: " << errString << endl;
	}
}

void idle()
{

}

void init()
{

	elevation = azimuth = 0;
	ballRadius = floorY = 4.0f;
	genNorms = false;

	// Enable depth test (visible surface determination)
	glEnable(GL_DEPTH_TEST);

	// OpenGL background color
	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);

	// Load/compile/link shaders and set to use for rendering
	ShaderInfo shaders[] = { { GL_VERTEX_SHADER, "Cube_Vertex_Shader.vert" },
	{ GL_FRAGMENT_SHADER, "Cube_Fragment_Shader.frag" },
	{ GL_NONE, NULL } };

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);

	//Get the shader parameter locations for passing data to shaders
	vertposition_loc = glGetAttribLocation(program, "vertexPosition");
	vertcolor_loc = glGetAttribLocation(program, "vertexColor");
	vertex_UV = glGetAttribLocation(program, "vertexUV");
	Matrix_loc = glGetUniformLocation(program, "Matrix");
	normal_loc = glGetAttribLocation(program, "normal");
	
	texture_location = glGetUniformLocation(program, "texture_Colors");

	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, texture_location);



	gmtl::identity(view);
	gmtl::identity(modelView);

	nearValue = 1.0f;
	farValue = 1000.0f;
	topValue = SCREEN_HEIGHT / SCREEN_WIDTH;
	bottomValue = topValue * -1.0f;
	rightValue = 1.0f;
	leftValue = -1.0f;
	
	normalMatrix.set(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	projection.set(
		((2.0f * nearValue) / (rightValue - leftValue)), 0.0f, ((rightValue + leftValue) / (rightValue - leftValue)), 0.0f,
		0.0f, ((2.0f * nearValue) / (topValue - bottomValue)), ((topValue + bottomValue) / (topValue - bottomValue)), 0.0f,
		0.0f, 0.0f, ((-1.0f * (farValue + nearValue)) / (farValue - nearValue)), ((-2.0f*farValue*nearValue)/(farValue-nearValue)),
		0.0f,0.0f,-1.0f,0.0f		
		);

	cameraZFactor = 60.f;

	cameraZ = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f,0.0f,cameraZFactor));
	cameraZ.setState(gmtl::Matrix44f::TRANS);
	
	view = cameraZ;
	gmtl::invert(view);

	buildGraph();
	
}

#pragma endregion

int main(int argc, char** argv)
{
	// For more details about the glut calls, 
	// refer to the OpenGL/freeglut Introduction handout.

	//Initialize the freeglut library
	glutInit(&argc, argv);

	//Specify the display mode
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	//Set the window size/dimensions
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);

	// Specify OpenGL version and core profile.
	// We use 3.3 in thie class, not supported by very old cards
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow("415/515 CUBOID DEMO");

	glewExperimental = GL_TRUE;

	if (glewInit())
		exit(EXIT_FAILURE);

	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutIdleFunc(idle);

	//Transfer the control to glut processing loop.
	glutMainLoop();

	return 0;
}