// For newer cards, use version 330 core or version 400 core
#version 330

// incoming vertex information
layout (location = 0) in vec4 vertexPosition; //If glVertexAttribPointer specifies vertices are 3D, OpenGL will add fourth coordinate w = 1 for this vec4 input.
layout (location = 1) in vec3 vertexColor;

// color to pass on (output)
out vec3 fragmentColor;

// matrix : uniform across primitive
uniform mat4 Matrix;

void main()
{
// apply application-provided transform
	gl_Position = Matrix * vertexPosition;
	
// Since we aren't using projection matrices yet, 
// the following adjusts for depth buffer "direction".
// Remove this once projection is added.
	gl_Position.z = -gl_Position.z;

// color is passed through to scan conversion and fragment shader
	fragmentColor = vertexColor;
}

