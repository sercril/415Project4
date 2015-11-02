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
VertexArrayObject::VertexArrayObject(std::vector<GLfloat> vertexData, std::vector<GLfloat> colorData, std::vector<GLfloat> normalData, std::vector<GLfloat> uvData, std::vector<GLushort> indexData, GLuint vertposition_loc, GLuint vertex_UV)
{

	this->vertex_data = vertexData;
	this->index_data = indexData;

	/*** VERTEX ARRAY OBJECT SETUP***/
	// Create/Generate the Vertex Array Object
	glGenVertexArrays(1, &this->vertexArray);
	// Bind the Vertex Array Object
	glBindVertexArray(this->vertexArray);

	// Create/Generate the Vertex Buffer Object for the vertices.
	glGenBuffers(1, &this->vertexBuffer);
	// Bind the Vertex Buffer Object.
	glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&vertexData[0])*vertexData.size(), &vertexData[0], GL_DYNAMIC_DRAW);
	// Specify data location and organization
	glVertexAttribPointer(vertposition_loc, // This number must match the layout in the shader
		3, // Size
		GL_FLOAT, // Type
		GL_FALSE, // Is normalized
		0, ((void*)0));
	glEnableVertexAttribArray(vertposition_loc);

	
	glGenBuffers(1, &this->uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&uvData[0])*uvData.size(), &uvData[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vertex_UV, 2, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertex_UV);


	
	glGenBuffers(1, &this->indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(&indexData[0])*indexData.size(),
		&indexData[0], GL_STATIC_DRAW);


	this->LoadVerticies(vertexData, normalData, uvData);
}

VertexArrayObject::~VertexArrayObject()
{
}

void VertexArrayObject::LoadVerticies(std::vector<GLfloat> vertexData, std::vector<GLfloat> normalData, std::vector<GLfloat> uvData)
{
	std::vector<GLfloat>::iterator vit;
	std::vector<GLfloat>::iterator nit;
	std::vector<GLfloat>::iterator uvit;
	for (vit = vertexData.begin(), nit = normalData.begin(), uvit = uvData.begin(); (vit < vertexData.end() || nit < normalData.end() || uvit < uvData.end()); vit += 3, nit += 3, uvit+=2)
	{
		Vertex v = Vertex(gmtl::Vec3f(*vit, *(vit + 1), *(vit + 2)), gmtl::Vec3f(*nit, *(nit + 1), *(nit + 2)), *(uvit), *(uvit+1));

		this->verticies.push_back(v);
	}
}

void VertexArrayObject::GenerateNormals()
{
	
	for (std::vector<Vertex>::iterator it = this->verticies.begin(); it < verticies.end(); ++it)
	{
		(*it).normal = gmtl::Vec3f(0.0f, 0.0f, 0.0f);
	}
	
	for (std::vector<GLushort>::iterator it = this->index_data.begin(); it < this->index_data.end(); it += 3)
	{
		Vertex v0, v1, v2;
		gmtl::Vec3f newNormal, vec1, vec2;


		v0 = this->verticies[*it];
		v1 = this->verticies[*(it+1)];
		v2 = this->verticies[*(it+2)];

		vec1 = v1.position - v0.position;
		vec2 = v2.position - v0.position;
	
		newNormal = gmtl::makeNormal(gmtl::makeCross(vec1,vec2));

		vec1 = v0.normal + newNormal;
		v0.normal = vec1;

		vec1 = v1.normal + newNormal;
		v1.normal = vec1;

		vec1 = v2.normal + newNormal;
		v2.normal = vec1;

	}

	for (std::vector<Vertex>::iterator it = this->verticies.begin(); it < verticies.end(); ++it)
	{
		(*it).normal = gmtl::makeNormal((*it).normal);
	}
	
}