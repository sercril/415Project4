#version 330

// Input vertex data
in vec3 vertexPosition;
in vec2 vertexUV;
in vec3 normal;

// Output texture coordinates data ; will be interpolated for each fragment.
out vec2 UV;

uniform mat4 Matrix;

void main(){
	// Output position of the vertex, in clip space : MVP * position
	gl_Position = Matrix * vec4(vertexPosition,1);

	// UV of the vertex. No special space for this one.
	UV = vertexUV;
}

