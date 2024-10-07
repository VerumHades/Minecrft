#version 330 core

in vec2 TexCoords;
in vec2 Size;
in float isText;
in vec3 Color;
in float isTexture;
in float textureIndex;
in vec4 borderWidth;
in vec3 borderColor;


out vec4 FragColor;

uniform sampler2DArray tex;
uniform sampler2D textAtlas;

void main()
{   
    bool isBorder = TexCoords.x < borderWidth.y || TexCoords.y < borderWidth.z || TexCoords.x > 1 - borderWidth.w || TexCoords.y > 1 - borderWidth.x;
    
    vec4 sampledText = vec4(1.0, 1.0, 1.0, texture(textAtlas, TexCoords).r);
    vec4 sampledTexture = texture(tex, vec3(TexCoords, textureIndex));

    FragColor = vec4(isBorder ? borderColor : Color, 1.0) * (isText > 0.5 ? sampledText : vec4(1));
    FragColor = isTexture > 0.5 ? sampledTexture : FragColor;
}