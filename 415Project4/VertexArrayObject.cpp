#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <map>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <gmtl\gmtl.h>
#include <gmtl\Matrix.h>


#include "VertexArrayObject.h"

using namespace std;


struct VectorLessThan : binary_function<gmtl::Vec3f,gmtl::Vec3f,bool>
{
	bool operator()(const gmtl::Vec3f& first, const gmtl::Vec3f& second)
	{
		return tie(first[0], first[1], first[2]) < tie(second[0], second[1], second[2]);
	}
};

VertexArrayObject::VertexArrayObject() {}
VertexArrayObject::VertexArrayObject(std::vector<GLfloat> vertexData, std::vector<GLfloat> colorData, std::vector<GLfloat> normalData, std::vector<GLfloat> uvData, std::vector<GLushort> indexData, GLuint vertposition_loc, GLuint vertex_UV, GLuint normal_loc, GLuint vertcolor_loc)
{

	this->vertex_data = vertexData;
	this->index_data = indexData;

	this->normal_loc = normal_loc;

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

	glGenBuffers(1, &this->colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&colorData[0])*colorData.size(), &colorData[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vertcolor_loc, 2, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertcolor_loc);

	glGenBuffers(1, &this->normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&normalData[0])*normalData.size(), &normalData[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(this->normal_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(this->normal_loc);
	
	glGenBuffers(1, &this->indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(&indexData[0])*indexData.size(),
		&indexData[0], GL_DYNAMIC_DRAW);


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
	
	std::vector<GLfloat> newNormals;
	std::map<gmtl::Vec3f, gmtl::Vec3f, VectorLessThan> pos2norm;
	

	cout << "shit happens" << endl;

	for (std::vector<Vertex>::iterator it = this->verticies.begin(); it < verticies.end(); ++it)
	{
		pos2norm[(*it).position] = gmtl::Vec3f(0.0f, 0.0f, 0.0f);
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
	
		newNormal = gmtl::makeCross(vec1,vec2);

		pos2norm[v0.position] += newNormal;
		pos2norm[v1.position] += newNormal;
		pos2norm[v2.position] += newNormal;

	}

	for (std::vector<Vertex>::iterator it = this->verticies.begin(); it < verticies.end(); ++it)
	{
		(*it).normal = gmtl::makeNormal(pos2norm[(*it).position]);		

		cout << (*it).normal << endl;

		newNormals.push_back((*it).normal[0]);
		newNormals.push_back((*it).normal[1]);
		newNormals.push_back((*it).normal[2]);
	}
	
	glGenBuffers(1, &this->normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(&newNormals[0])*newNormals.size(), &newNormals[0], GL_DYNAMIC_DRAW);
	glVertexAttribPointer(this->normal_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(this->normal_loc);

}