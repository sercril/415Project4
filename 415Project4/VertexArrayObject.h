#ifndef __VERTEX_ARRAY_OBJECT_H__
#define __VERTEX_ARRAY_OBJECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>

#include "Vertex.h"
#include "Texture.h"

using namespace std;

class VertexArrayObject
{
	public:
		VertexArrayObject();
		VertexArrayObject(std::vector<GLfloat> vertexData, std::vector<GLfloat> colorData, std::vector<GLfloat> normalData, std::vector<GLfloat> uvData, std::vector<GLushort> indexData, GLuint vertposition_loc, GLuint vertex_UV, GLuint normal_loc);
		~VertexArrayObject();

		void GenerateNormals();

		GLuint vertexArray;
		std::vector<Vertex> verticies;

	private:
		void LoadVerticies(std::vector<GLfloat> vertexData, std::vector<GLfloat> normalData, std::vector<GLfloat> uvData);

		GLuint vertexBuffer, colorBuffer, indexBuffer, uvBuffer, normalBuffer, textureLocation;

		std::vector<GLfloat> vertex_data;
		std::vector<GLushort> index_data;

};



#endif __VERTEX_ARRAY_OBJECT_H__