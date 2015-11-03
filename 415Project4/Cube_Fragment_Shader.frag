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



void main(){

	vec3 normNormal, V, R, lightDirection, normLight;
	float lightDotNormal;

	//normLight = normalize(lightPosition);

	lightDirection = normalize( lightPosition - vec3(fragmentPosition));

	V = normalize(-vec3(fragmentPosition));

	normNormal = normalize(fragmentNormal);

	lightDotNormal = dot(lightDirection, normNormal);

	R = normalize(2 * lightDotNormal * normNormal - lightDirection);
	
	color = texture2D( texture_Colors, UV ).rgb * dot(fragmentNormal, lightDirection);

	color = ambientLight 
			* texture2D( texture_Colors, UV ).rgb 
			* (0.5f + 0.5f * dot(normNormal, normalize(upVector.xyz)))
			+ diffuseLight
			* texture2D( texture_Colors, UV ).rgb
			* (max(0.0f, dot(normNormal,lightDirection))) + specCoefficient * specularLight * pow(max(0.0f,dot(V,R)), shine);

	//color = specCoefficient * specularLight * pow(max(0.0f,dot(V,R)), shine);
	//color = lightDirection;

}
