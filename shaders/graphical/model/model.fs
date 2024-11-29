#version 330 core

precision highp float; 

in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D textureIn;

void main()
{
    FragColor = texture(textureIn, TexCoords);
}