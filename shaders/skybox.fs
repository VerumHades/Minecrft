#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 sunPos;
uniform vec3 camDir;
uniform vec3 camPos;
void main()
{    
    FragColor = texture(skybox, TexCoords);
}