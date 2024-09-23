#version 330 core

in vec2 TexCoords;
in vec2 Size;
in float isText;
in vec3 Color;
in float isTexture;
in float textureIndex;
out vec4 FragColor;

const float borderWidth = 0.01;

uniform sampler2DArray tex;
uniform sampler2D textAtlas;

void main()
{   
    bool border = TexCoords.x < borderWidth || TexCoords.y < borderWidth || TexCoords.x > 1 - borderWidth || TexCoords.y > 1 - borderWidth;
    
    float isBorder = border ? 0 : 1;

    vec4 sampledText = vec4(1.0, 1.0, 1.0, texture(textAtlas, TexCoords).r);
    vec4 sampledTexture = texture(tex, vec3(TexCoords, textureIndex));

    FragColor = vec4(Color, 1.0) * (isText > 0.5 ? sampledText : vec4(1));
    FragColor = isTexture > 0.5 ? sampledTexture : FragColor;
}