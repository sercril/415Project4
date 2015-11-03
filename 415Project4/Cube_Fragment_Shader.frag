#version 330 core

// Texture coordinate values from the vertex shaders
in vec2 UV;
in vec3 fragmentNormal;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D texture_Colors;
uniform vec4 a_parameter;

void main(){

	color = texture2D( texture_Colors, UV ).rgb * dot(fragmentNormal, vec3(a_parameter));
}
