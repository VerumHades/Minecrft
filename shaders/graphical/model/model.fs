#version 330 core

precision highp float; 

in vec3 FragPos;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D textureIn;

void main()
{
    FragColor = vec4(1.0,1.0,0.5, 1.0);
}