#version 330 core

// Texture coordinate values from the vertex shaders
in vec2 UV;
in vec3 fragmentNormal;
in vec4 fragmentPosition;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D texture_Colors;
uniform vec3 lightPosition;
uniform vec4 upVector;
uniform vec3 ambientLight;
uniform vec3 diffuseLight;
uniform vec3 specularLight;

uniform float specCoefficient;
uniform float shine;

uniform int ambientFlag;
uniform int diffuseFlag;
uniform int specFlag;

void main(){

	vec3 normNormal, V, R, lightDirection, normLight;
	float lightDotNormal;

	//normLight = normalize(lightPosition);

	lightDirection = normalize( lightPosition - fragmentPosition.xyz);

	V = normalize(-fragmentPosition.xyz);

	normNormal = normalize(fragmentNormal);

	lightDotNormal = dot(normNormal, lightDirection);

	R = normalize(2 * lightDotNormal * normNormal - lightDirection);

	color.rgb = vec3(0,0,0);
	
	if(ambientFlag == 1)
	{
		color = color + ambientLight * texture2D( texture_Colors, UV ).rgb * (0.5f + 0.5f * dot(normNormal, normalize(upVector.xyz)));
	}

	if(diffuseFlag == 1)
	{
		color = color + diffuseLight * texture2D( texture_Colors, UV ).rgb * (max(0.0f, dot(normNormal,lightDirection)));
	}

	if(specFlag == 1)
	{
		color = color + specCoefficient * specularLight * pow(max(0.0f,dot(V,R)), shine);
	}


}
