#version 330 core

precision highp float; 

in vec3 Normals;
in vec2 TexCoords;

in vec3 crntPosition;

out vec4 FragColor;

uniform sampler2D texture1;

const vec3 lightPosition = vec3(0,1000,0);
const vec3 lightColor = vec3(0.1,0.5,0.5);

void main()
{
    vec3 normal = normalize(Normals);
    vec3 lightDirection = normalize(lightPosition - crntPosition);

    float diffuse = max(dot(normal, lightDirection), 0.0);

    FragColor = texture(texture1, TexCoords);// * diffuse;
    //FragColor.a = 1.0;
    //FragColor = vec4(vec3(0.5,0.5,0.2) - Normals, 1);
}