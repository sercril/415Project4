#ifndef __SCENE_OBJECT_H__
#define __SCENE_OBJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <array>


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>

#include "VertexArrayObject.h"

using namespace std;

class SceneObject
{

public:

	SceneObject();
	SceneObject(float length, float width, float depth, GLuint vertposition_loc, GLuint vertcolor_loc);
	SceneObject(float radius, std::vector<GLfloat> vertex_data, std::vector<GLfloat> normal_data, std::vector<GLfloat> uv_data, std::vector<GLushort> index_data, GLuint vertposition_loc, GLuint vertcolor_loc);
	~SceneObject();

	gmtl::Matrix44f matrix, scale;
	float length, width, depth, radius;
	VertexArrayObject VAO;


private:

};

#endif __SCENE_OBJECT_H__