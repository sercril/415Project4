#ifndef __VERTEX_ARRAY_OBJECT_H__
#define __VERTEX_ARRAY_OBJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>

using namespace std;

class VertexArrayObject
{
	public:
		VertexArrayObject();
		VertexArrayObject(std::vector<GLfloat> vertexData, std::vector<GLfloat> colorData, std::vector<GLushort> indexData, GLuint vertposition_loc, GLuint vertcolor_loc);
		~VertexArrayObject();

		GLuint vertexArray;

	private:
		GLuint vertexBuffer, colorBuffer, indexBuffer;
};



#endif __VERTEX_ARRAY_OBJECT_H__