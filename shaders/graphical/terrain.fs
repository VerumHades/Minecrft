#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

precision highp float; 

in vec3 Normal;
in vec2 TexCoords;
in float TexIndex;

in vec4 FragPosLightSpace;
in vec3 FragPos;
in vec3 pos;
in float occlusion;

uniform sampler2DArray textureArray;
uniform sampler2D shadowMap;
uniform vec3 sunDir;
//uniform sampler3D lightArray;

/*float ShadowCalculation(vec4 fragPosLightSpace)
{   
    // perform perspective divide
    vec4 projCoords = fragPosLightSpace / fragPosLightSpace.w;
    //projCoords.xyz += Normal * 0.0005;
   // projCoords.xyz += -Normal * 0.0005;

    //float dist = length(projCoords.xy) + .1;
    //projCoords.xy /= dist;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    //projCoords.xy = projCoords.xy * 0.5 + 0.5;
    //projCoords.z = projCoords.z * 0.5 + 0.5;

    //projCoords.z -= acos(dot(Normal, sunDir)) * 0.0005;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    float bias = max(0.0002 * (1.0 - dot(Normal, sunDir)), 0.00001);  

    //float bias = 0.0002;
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  

    return shadow;
} */

void main()
{
    vec4 full_color = texture(textureArray, vec3(TexCoords, TexIndex));
    if(full_color.a < 0.1) discard;
    vec3 color = full_color.rgb;

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = full_color.rgb;
    gAlbedoSpec.a = full_color.r;
}