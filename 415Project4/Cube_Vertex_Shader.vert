#version 330

// Input vertex data
in vec4 vertexPosition;
in vec2 vertexUV;
in vec3 vertexNormal;

// Output texture coordinates data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 fragmentNormal;
out vec4 fragmentPosition;

uniform mat4 Matrix;
uniform mat4 NormalMatrix;
uniform mat4 modelview;



void main(){
	// Output position of the vertex, in clip space : MVP * position
	gl_Position = Matrix * vertexPosition;

	mat3 matrixDummy = transpose(inverse(mat3(modelview)));

	// UV of the vertex. No special space for this one.
	UV = vertexUV;

	fragmentNormal = (matrixDummy * vertexNormal).xyz;

	fragmentPosition = (modelview * vertexPosition);
}

