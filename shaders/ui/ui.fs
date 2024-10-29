#version 330 core

in vec2 TexCoords;
in vec2 Size;
in float isText;
in vec4 Color;
in float isTexture;
in float textureIndex;
in vec4 borderWidth;

in vec4 clipRegion;
in vec3 screenPos;

in vec4 borderColorTop;
in vec4 borderColorRight;
in vec4 borderColorBottom;
in vec4 borderColorLeft;

out vec4 FragColor;

uniform sampler2DArray tex;
uniform sampler2D textAtlas;

void main()
{    
    
    if(screenPos.x < clipRegion.x || screenPos.y < clipRegion.y || screenPos.x > clipRegion.z || screenPos.y > clipRegion.w) discard;
    
    bool isBorder = TexCoords.x < borderWidth.y || TexCoords.y < borderWidth.z || TexCoords.x > 1 - borderWidth.w || TexCoords.y > 1 - borderWidth.x;
    
    vec4 borderColor = vec4(1.0,0,0,1.0);

    if     (TexCoords.y > 1 - borderWidth.x) borderColor = borderColorTop;
    else if(TexCoords.y < borderWidth.z    ) borderColor = borderColorBottom;
    else if(TexCoords.x > 1 - borderWidth.y) borderColor = borderColorRight;
    else if(TexCoords.x < borderWidth.w    ) borderColor = borderColorLeft;

    vec4 sampledText = vec4(1.0, 1.0, 1.0, texture(textAtlas, TexCoords).r);
    vec4 sampledTexture = texture(tex, vec3(TexCoords, textureIndex));

    FragColor = (isBorder ? borderColor : Color) * (isText > 0.5 ? sampledText : vec4(1));
    FragColor = isTexture > 0.5 ? sampledTexture : FragColor;

}