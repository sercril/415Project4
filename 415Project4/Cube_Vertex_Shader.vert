#version 330

// Input vertex data
in vec3 vertexPosition;
in vec2 vertexUV;
in vec3 vertexNormal;

// Output texture coordinates data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 fragmentNormal;

uniform mat4 Matrix;
uniform mat4 NormalMatrix;

void main(){
	// Output position of the vertex, in clip space : MVP * position
	gl_Position = Matrix * vec4(vertexPosition,1);

	// UV of the vertex. No special space for this one.
	UV = vertexUV;

	fragmentNormal = (NormalMatrix * vec4(vertexNormal,0)).xyz;
	//fragmentNormal = vertexNormal;
}

