// For newer cards, use "#version 330 core" or "#version 400 core"
#version 330

// color from scan conversion (interpolated from vertex shader output)
in vec3 fragmentColor;

// final color to write into output buffer
out vec3 color;

void main(){
	color = fragmentColor;
}