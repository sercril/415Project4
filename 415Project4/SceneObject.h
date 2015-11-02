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

struct Texture
{
	unsigned int textureWidth, textureHeight;
	unsigned char *imageData;

	Texture() {}
	Texture(unsigned int textureWidth, unsigned int textureHeight, unsigned char *imageData)
	{
		this->textureWidth = textureWidth;
		this->textureHeight = textureHeight;
		this->imageData = imageData;
	}
};

class SceneObject
{

public:

	SceneObject();
	SceneObject(float length, float width, float depth, GLuint vertposition_loc, GLuint vertex_UV);
	SceneObject(float radius, 
				std::vector<GLfloat> vertex_data, 
				std::vector<GLfloat> normal_data, 
				std::vector<GLfloat> uv_data, 
				std::vector<GLushort> index_data, 
				GLuint vertposition_loc, 
				GLuint vertex_UV);
	~SceneObject();

	void SetTexture(Texture t);

	gmtl::Matrix44f matrix, scale;
	float length, width, depth, radius;
	VertexArrayObject VAO;

	Texture texture;

private:

};

#endif __SCENE_OBJECT_H__