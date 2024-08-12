#version 330 core

precision highp float; 

in vec3 Normals;
in vec2 TexCoords;
in float TexIndex;
in vec3 LightLevel;

in vec3 crntPosition;
in vec3 pos;

out vec4 FragColor;

uniform sampler2DArray textureArray;
//uniform sampler3D lightArray;

void main()
{
    //FragColor = texture(texture1, TexCoords) * vec4(LightLevel,1); //* diffuse;
    
    FragColor = texture(textureArray, vec3(TexCoords, TexIndex));// * texture(lightArray, pos / vec3(64,256,64));
    //FragColor.a = 1.0;
    //FragColor = vec4(vec3(0.5,0.5,0.2) - Normals, 1);
}