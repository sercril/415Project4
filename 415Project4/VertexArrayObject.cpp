#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>

#include "VertexArrayObject.h"

using namespace std;


VertexArrayObject::VertexArrayObject() {}
VertexArrayObject::VertexArrayObject(std::vector<GLfloat> vertexData, std::vector<GLfloat> colorData, std::vector<GLushort> indexData, GLuint vertposition_loc, GLuint vertcolor_loc)
{
	/*** VERTEX ARRAY OBJECT SETUP***/
	// Create/Generate the Vertex Array Object
	glGenVertexArrays(1, &this->vertexArray);
	// Bind the Vertex Array Object
	glBindVertexArray(this->vertexArray);

	// Create/Generate the Vertex Buffer Object for the vertices.
	glGenBuffers(1, &this->vertexBuffer);
	// Bind the Vertex Buffer Object.
	glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&vertexData[0])*vertexData.size(), &vertexData[0], GL_STATIC_DRAW);
	// Specify data location and organization
	glVertexAttribPointer(vertposition_loc, // This number must match the layout in the shader
		3, // Size
		GL_FLOAT, // Type
		GL_FALSE, // Is normalized
		0, ((void*)0));
	// Enable the use of this array
	glEnableVertexAttribArray(vertposition_loc);

	// Similarly, set up the color buffer.
	glGenBuffers(1, &this->colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&colorData[0])*colorData.size(), &colorData[0], GL_STATIC_DRAW);
	glVertexAttribPointer(vertcolor_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertcolor_loc);

	// Set up the element (index) array buffer and copy in data
	glGenBuffers(1, &this->indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);
	// Transfer data
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(&indexData[0])*indexData.size(),
		&indexData[0], GL_STATIC_DRAW);


}

VertexArrayObject::~VertexArrayObject()
{
}

