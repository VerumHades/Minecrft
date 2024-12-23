#version 330 core

in vec2 TexCoords;
in vec4 Color;
in float Type;

out vec4 FragColor;

uniform sampler2DArray tex;
uniform sampler2D textAtlas;

void main()
{    
    vec4 sampledText = vec4(1.0, 1.0, 1.0, texture(textAtlas, TexCoords).r);
    vec4 sampledTexture = texture(tex, vec3(TexCoords, 0));

    FragColor = 
        Type < 0.5 ? Color : 
        Type < 1.5 ? sampledText * Color :
        Type < 2.5 ? sampledTexture * Color : vec4(1.0,0,0,1.0);

    //FragColor = vec4(TexCoords,0.0,1.0);
}