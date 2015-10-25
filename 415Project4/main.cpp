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

#pragma comment (lib, "glew32.lib")
#pragma warning (disable : 4996) // Windows ; consider instead replacing fopen with fopen_s

using namespace std;

#pragma region Structs and Enums

enum ObjectType
{
	FLOOR = 0,
	BALL,
	PALM,
	METACARPAL,
	PROXIMAL,
	MIDDLE,
	DISTAL
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

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024
//#define WORLD_OBJECTS 3
#define NUM_OBJECTS 18
#define INDECIES 10000

// Parameter location for passing a matrix to vertex shader
GLuint Matrix_loc;
// Parameter locations for passing data to shaders
GLuint vertposition_loc, vertcolor_loc;

GLenum errCode;
const GLubyte *errString;

int mouseX, mouseY,
mouseDeltaX, mouseDeltaY;

float azimuth, elevation, startTime, hand[16][3], ballRadius, floorY, attachments[4], thumbLoc[3], viewScaleFactor;

gmtl::Matrix44f view, modelView, ballTransform, floorTransform, palmTransform, viewScale, camera, ballScale,
				palmRotM, ballRotM,
				elevationRotation, azimuthRotation;
gmtl::Quatf palmRotQ, ballRotQ;

std::vector<SceneNode*> sceneGraph;

std::vector<GLfloat> ball_vertex_data;
std::vector<GLushort> ball_index_data;

std::vector<gmtl::Matrix44f> fingerTransforms[15];

FILE *animFP;

struct Keyframe c;

std::vector<Keyframe> keyframes;

bool restart;
#pragma endregion

#pragma region Helper Functions

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

	gmtl::transpose(elevationRotation);
	gmtl::transpose(azimuthRotation);
	
	view = viewScale * elevationRotation * azimuthRotation;

	view.setState(gmtl::Matrix44f::FULL);

	glutPostRedisplay();
}

void readGeometry()
{
	ifstream fp;
	int i = 0;

	fp.open("geometryfileexample.txt", ios_base::in);

	if (fp)
	{
		for (std::string line; std::getline(fp, line); ++i)
		{
			std::istringstream in(line);

			if (i < 16)
			{
				in >> hand[i][0] >> hand[i][1] >> hand[i][2];
			}
			else if (i == 16)
			{
				in >> thumbLoc[0] >> thumbLoc[1] >> thumbLoc[2];
			}
			else if (i == 17)
			{
				in >> attachments[0] >> attachments[1] >> attachments[2] >> attachments[3];
			}
			else if (i == 18)
			{
				in >> ballRadius;
			}
			else if (i == 19)
			{
				in >> floorY;
			}
			
		}
		

		fp.close();
	}
}

void importBallData()
{
	ifstream fp;
	int i = 0, j = 0, numVerticies, numIndicies, numPolygons;

	fp.open("SphereMesh.txt", ios_base::in);

	if (fp)
	{

		for (std::string line; std::getline(fp, line); ++i)
		{
			std::istringstream in(line);

			if (i == 0)
			{
				in >> numVerticies;
				ball_vertex_data.resize(numVerticies*3);
			}
			else if (i > 0 && i <= numVerticies)
			{
				in >> ball_vertex_data[j] >> ball_vertex_data[j+1] >> ball_vertex_data[j+2];
				j += 3;
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

SceneNode * buildFinger(int finger)
{
	SceneNode* metacarpal = new SceneNode();
	SceneNode* proximal = new SceneNode();
	SceneNode* middle = new SceneNode();
	SceneNode* distal = new SceneNode();
	SceneNode* returnNode = new SceneNode();
	int base;
	gmtl::Matrix44f initialTranslation;

	if (finger == 0)
	{
		metacarpal->parent = sceneGraph[0];
		metacarpal->type = METACARPAL;
		metacarpal->object = SceneObject(hand[finger + 1][0], hand[finger + 1][1], hand[finger + 1][2], vertposition_loc, vertcolor_loc);
		metacarpal->id = finger;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(thumbLoc[0], thumbLoc[1], thumbLoc[2]));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		metacarpal->object.matrix =  initialTranslation;

		proximal->type = PROXIMAL;
		proximal->parent = metacarpal;
		proximal->object = SceneObject(hand[finger + 2][0], hand[finger + 2][1], hand[finger + 2][2], vertposition_loc, vertcolor_loc);
		proximal->id = finger + 1;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(proximal->parent->object.length, 0.0f, 0.0f));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		proximal->object.matrix =  initialTranslation;

		distal->type = DISTAL;
		distal->parent = proximal;
		distal->object = SceneObject(hand[finger + 3][0], hand[finger + 3][1], hand[finger + 3][2], vertposition_loc, vertcolor_loc);
		distal->children.clear();
		distal->id = finger + 2;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(distal->parent->object.length, 0.0f, 0.0f));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		distal->object.matrix = initialTranslation;

		proximal->children.push_back(distal);
		metacarpal->children.push_back(proximal);
		
		returnNode = metacarpal;

	}
	else
	{
		base = finger * 3;
		
		proximal->type = PROXIMAL;
		proximal->parent = sceneGraph[0];
		proximal->object = SceneObject(hand[base + 1][0], hand[base + 1][1], hand[base + 1][2], vertposition_loc, vertcolor_loc);
		proximal->id = base;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(proximal->parent->object.length, attachments[finger-1], 0.0f));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		proximal->object.matrix = initialTranslation;

		middle->type = MIDDLE;
		middle->parent = proximal;
		middle->object = SceneObject(hand[base + 2][0], hand[base + 2][1], hand[base + 2][2], vertposition_loc, vertcolor_loc);
		middle->id = base + 1;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(middle->parent->object.length, 0.0f, 0.0f));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		middle->object.matrix = initialTranslation;

		distal->type = DISTAL;
		distal->parent = middle;
		distal->object = SceneObject(hand[base + 3][0], hand[base + 3][1], hand[base + 3][2], vertposition_loc, vertcolor_loc);
		distal->children.clear();
		distal->id = base + 2;
		initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(distal->parent->object.length, 0.0f, 0.0f));
		initialTranslation.setState(gmtl::Matrix44f::TRANS);
		distal->object.matrix =  initialTranslation;

		middle->children.push_back(distal);
		proximal->children.push_back(middle);

		returnNode = proximal;

	}

	return returnNode;
}

void buildGraph()
{
	
	SceneNode* palm = new SceneNode();
	SceneNode* ball = new SceneNode();
	SceneNode* floor = new SceneNode();
	gmtl::Matrix44f initialTranslation, moveLeft;

	readGeometry();

	//Hand
	palm->type = PALM;
	palm->parent = NULL;
	palm->object = SceneObject(hand[0][0], hand[0][1], hand[0][2], vertposition_loc, vertcolor_loc);
	palm->children = std::vector<SceneNode*>(5);
		
	sceneGraph.push_back(palm);
	
	for (int i = 0; i < 5; ++i)
	{		
		palm->children[i] = buildFinger(i);
	}
	
	//Ball
	importBallData();
	ball->type = BALL;
	ball->parent = NULL;
	ball->object = SceneObject(ballRadius, ball_vertex_data, ball_index_data, vertposition_loc, vertcolor_loc);
	ball->children.clear();
	sceneGraph.push_back(ball);

	//Floor
	floor->type = FLOOR;
	floor->parent = NULL;
	floor->object = SceneObject(ballRadius * 10, 1.0f, ballRadius*10, vertposition_loc, vertcolor_loc);
	floor->children.clear();
	initialTranslation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f, floorY, 0.0f));
	initialTranslation.setState(gmtl::Matrix44f::TRANS);
	//Make it look good
	moveLeft = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(((ballRadius* 10)*-1.0f)/2, 0.0f, 0.0f));
	moveLeft.setState(gmtl::Matrix44f::TRANS);
	initialTranslation = moveLeft * initialTranslation;
	floor->object.matrix = floor->object.matrix * initialTranslation;

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

			switch (graph[i]->type)
			{
				case BALL:
					newMV = newMV * ballScale;
					break;
				case PALM:
				case METACARPAL:
				case PROXIMAL:
				case MIDDLE:
				case DISTAL:				
				case FLOOR:
					break;
			}

			//Render
			renderTransform = newMV;
			
			glBindVertexArray(graph[i]->object.vertex_array);
			// Send a different transformation matrix to the shader
			glUniformMatrix4fv(Matrix_loc, 1, GL_FALSE, &renderTransform[0][0]);

			// Draw the transformed cuboid
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFF);
			glDrawElements((graph[i]->type == BALL) ? GL_TRIANGLES : GL_TRIANGLE_STRIP, INDECIES, GL_UNSIGNED_SHORT, NULL);

			/*switch (graph[i]->type)
			{
				case METACARPAL:
				case PROXIMAL:
				case MIDDLE:
				case DISTAL:
					gmtl::invertFull(inverseTransform, graph[i]->object.matrix * thisTransform);
					modelView = modelView * inverseTransform;
					break;
			}*/

			
			if (!graph[i]->children.empty())
			{
				renderGraph(graph[i]->children, newMV);
			}

			
			
		}
	}
	
	return;
}

void importAnimation()
{
	if (animFP = fopen("animdata.bin", "rb"))
	{
		while (fread((void *)&c, sizeof(c), 1, animFP) == 1)
		{
			keyframes.push_back(c);
		}
		fclose(animFP);
	}
}

Keyframe interpolate(float currentTime, Keyframe currentFrame, Keyframe nextFrame)
{
	Keyframe iFrame;
	gmtl::Quatf normQuat;
	float s;

	s = (currentTime - currentFrame.time) / (nextFrame.time-currentFrame.time);

	for (int i = 0; i < 3; ++i)
	{
		iFrame.palm_p[i] = (1.0f - s)*currentFrame.palm_p[i] + (s*nextFrame.palm_p[i]);
	}
	for (int i = 0; i < 4; ++i)
	{
		iFrame.palm_q[i] = (1.0f - s)*currentFrame.palm_q[i] + (s*nextFrame.palm_q[i]);
	}

	normQuat = gmtl::Quatf(iFrame.palm_q[0], iFrame.palm_q[1], iFrame.palm_q[2], iFrame.palm_q[3]);
	gmtl::normalize(normQuat);

	for (int i = 0; i < 4; ++i)
	{
		iFrame.palm_q[i] = normQuat[i];
	}

	for (int i = 0; i < 16; ++i)
	{
		iFrame.joint[i] = (1.0f - s)*currentFrame.joint[i] + (s*nextFrame.joint[i]);
	}

	for (int i = 0; i < 3; ++i)
	{
		iFrame.ball_p[i] = (1.0f - s)*currentFrame.ball_p[i] + (s*nextFrame.ball_p[i]);
	}
	for (int i = 0; i < 4; ++i)
	{
		iFrame.ball_q[i] = (1.0f - s)*currentFrame.ball_q[i] + (s*nextFrame.ball_q[i]);
	}

	normQuat = gmtl::Quatf(iFrame.ball_q[0], iFrame.ball_q[1], iFrame.ball_q[2], iFrame.ball_q[3]);
	gmtl::normalize(normQuat);

	for (int i = 0; i < 4; ++i)
	{
		iFrame.ball_q[i] = normQuat[i];
	}

	return iFrame;
}

#pragma endregion

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


	elevation += degreesToRadians(arcToDegrees(mouseDeltaY)) / SCREEN_HEIGHT;
	azimuth += degreesToRadians(arcToDegrees(mouseDeltaX)) / SCREEN_WIDTH;

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

		case 'r':
			restart = true;
			break;

		case 'z':
			viewScaleFactor = viewScaleFactor + 0.005f;
			viewScale = gmtl::makeScale<gmtl::Matrix44f>(gmtl::Vec3f(viewScaleFactor, viewScaleFactor, viewScaleFactor));
			view = viewScale *elevationRotation * azimuthRotation;
			break;

		case 'Z':
			viewScaleFactor = viewScaleFactor - 0.005f;
			viewScale = gmtl::makeScale<gmtl::Matrix44f>(gmtl::Vec3f(viewScaleFactor, viewScaleFactor, viewScaleFactor));
			view = viewScale *elevationRotation * azimuthRotation;
			break;

		case 'q': case 'Q': case 033 /* Escape key */:
			exit(EXIT_SUCCESS);
			break;
	}
}

#pragma endregion

void display()
{
	gmtl::Matrix44f tempModelView;

	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	tempModelView = modelView;
	renderGraph(sceneGraph, view);

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

	gmtl::Matrix44f flexion, abduction, thumbRoll, thumbX, translation; 
	static std::vector<Keyframe>::iterator i = keyframes.begin();
	float time = glutGet(GLUT_ELAPSED_TIME);
	struct Keyframe c, thisFrame, nextFrame;

	if (restart)
	{
		startTime = time;
		i = keyframes.begin();
		sceneGraph.clear();
		buildGraph();
		restart = false;
	}

	if (i == keyframes.end())
	{
		return;
	}

	while ((i+1) != (keyframes.end()-1) && (time-startTime) > i->time)
	{
		++i;
		
		
	}

	thisFrame = *i;
	nextFrame = *(i + 1);

	if ((i + 1) != (keyframes.end() - 1))
	{
		c = interpolate((time - startTime), thisFrame, nextFrame);
	}
	else
	{
		c = *(keyframes.end() - 1);
	}
	
	
	
	
	

	/*if ((i + 1) != keyframes.end())
	{
		
	}
	else
	{
		thisFrame = *(keyframes.end() -1);
		nextFrame = *(keyframes.end());
	}*/
		
	

	

	

	#pragma region "Palm"

	palmTransform = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(c.palm_p[0], c.palm_p[1], c.palm_p[2]));
	palmTransform.setState(gmtl::Matrix44f::TRANS);

	palmRotQ = gmtl::Quatf(c.palm_q[0], c.palm_q[1], c.palm_q[2], c.palm_q[3]);
	palmRotM = gmtl::make<gmtl::Matrix44f>(palmRotQ);
	palmRotM.setState(gmtl::Matrix44f::ORTHOGONAL);

	sceneGraph[0]->object.matrix = palmTransform * palmRotM;

	#pragma endregion
	
	#pragma region "Thumb"

	thumbRoll = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(c.joint[0], 0.0f, 0.0f));
	thumbRoll.setState(gmtl::Matrix44f::ORTHOGONAL);

	abduction = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, 0.0f, c.joint[3]));
	abduction.setState(gmtl::Matrix44f::ORTHOGONAL);

	thumbX = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(gmtl::Math::deg2Rad(-45.0f), 0.0f, 0.0f));
	thumbX.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(thumbLoc[0], thumbLoc[1], thumbLoc[2]));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[0]->object.matrix = translation * thumbRoll * abduction * thumbX;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[1], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[0]->children[0]->object.matrix = translation * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[2], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[0]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[0]->children[0]->children[0]->object.matrix = translation * flexion;

	#pragma endregion
	
	#pragma region "Index"

	abduction = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, 0.0f, c.joint[6]));
	abduction.setState(gmtl::Matrix44f::ORTHOGONAL);

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[4], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[1]->parent->object.length, attachments[0], 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);
	
	sceneGraph[0]->children[1]->object.matrix = translation * abduction * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[5], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[1]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[1]->children[0]->object.matrix = translation * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, ((2.0f*c.joint[5]) / 3.0f), 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[1]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[1]->children[0]->children[0]->object.matrix = translation * flexion;

	#pragma endregion

	#pragma region "Middle"

	abduction = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, 0.0f, c.joint[9]));
	abduction.setState(gmtl::Matrix44f::ORTHOGONAL);

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[7], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[2]->parent->object.length, attachments[1], 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[2]->object.matrix = translation * abduction * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[8], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[2]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[2]->children[0]->object.matrix = translation * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, ((2.0f*c.joint[8]) / 3.0f), 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[2]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[2]->children[0]->children[0]->object.matrix = translation * flexion;

	#pragma endregion

	#pragma region "Ring"

	abduction = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, 0.0f, c.joint[12]));
	abduction.setState(gmtl::Matrix44f::ORTHOGONAL);

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[10], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[3]->parent->object.length, attachments[2], 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[3]->object.matrix = translation * abduction * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[11], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[3]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[3]->children[0]->object.matrix = translation * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, ((2.0f*c.joint[11]) / 3.0f), 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[3]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[3]->children[0]->children[0]->object.matrix = translation * flexion;


	#pragma endregion

	#pragma region "Pinky"

	abduction = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, 0.0f, c.joint[15]));
	abduction.setState(gmtl::Matrix44f::ORTHOGONAL);

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[13], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[4]->parent->object.length, attachments[3], 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[4]->object.matrix = translation * abduction * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, c.joint[14], 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[4]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[4]->children[0]->object.matrix = translation * flexion;

	flexion = gmtl::makeRot<gmtl::Matrix44f>(gmtl::EulerAngleXYZf(0.0f, ((2.0f*c.joint[14]) / 3.0f), 0.0f));
	flexion.setState(gmtl::Matrix44f::ORTHOGONAL);

	translation = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(sceneGraph[0]->children[4]->children[0]->children[0]->parent->object.length, 0.0f, 0.0f));
	translation.setState(gmtl::Matrix44f::TRANS);

	sceneGraph[0]->children[4]->children[0]->children[0]->object.matrix = flexion;

	#pragma endregion

	#pragma region "Ball"

	ballTransform = gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(c.ball_p[0], c.ball_p[1], c.ball_p[2]));
	ballTransform.setState(gmtl::Matrix44f::TRANS);

	ballRotQ = gmtl::Quatf(c.ball_q[0], c.ball_q[1], c.ball_q[2], c.ball_q[3]);
	ballRotM = gmtl::make<gmtl::Matrix44f>(ballRotQ);
	ballRotM.setState(gmtl::Matrix44f::ORTHOGONAL);

	sceneGraph[1]->object.matrix = ballTransform * ballRotM;

	#pragma endregion
		

	glutPostRedisplay();
	
	

}



void init()
{

	elevation = azimuth = 0;

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
	Matrix_loc = glGetUniformLocation(program, "Matrix");

	gmtl::identity(view);
	gmtl::identity(modelView);
	gmtl::identity(ballTransform);
	gmtl::identity(floorTransform);
	gmtl::identity(palmTransform);

	for (int i = 0; i < 15; ++i)
	{
		fingerTransforms[i].push_back(palmTransform);
	}
	viewScaleFactor = 0.02f;
	viewScale = gmtl::makeScale<gmtl::Matrix44f>(gmtl::Vec3f(viewScaleFactor, viewScaleFactor, viewScaleFactor));

	viewScale.setState(gmtl::Matrix44f::AFFINE);
	gmtl::invert(viewScale);

	view = viewScale;

	buildGraph();
	
	ballScale = gmtl::makeScale<gmtl::Matrix44f>(gmtl::Vec3f(ballRadius, ballRadius, ballRadius));
	ballScale.setState(gmtl::Matrix44f::AFFINE);

	importAnimation();

	restart = false;
	startTime = 0;
}

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