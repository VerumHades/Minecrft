#version 330 core

precision highp float; 

in vec3 Normal;
in vec2 TexCoords;

in vec4 FragPosLightSpace;
in vec3 FragPos;
in vec3 pos;

out vec4 FragColor;

uniform sampler2D textureIn;
uniform sampler2D shadowMap;
uniform vec3 sunDir;
//uniform sampler3D lightArray;

float ShadowCalculation(vec4 fragPosLightSpace)
{   
    // perform perspective divide
    vec4 projCoords = fragPosLightSpace / fragPosLightSpace.w;
    
    float dist = length(projCoords.xy) + .1;
    projCoords.xy /= dist;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    //projCoords.xy = projCoords.xy * 0.5 + 0.5;
    //projCoords.z = projCoords.z * 0.5 + 0.5;

    //projCoords.z += 0.0005;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.001;  
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
} 

void main()
{
    vec3 color = texture(textureIn,TexCoords).rgb;
    vec3 normal = normalize(-Normal);
    vec3 lightColor = vec3(1.0);
    // ambient
    vec3 ambient = 0.4 * lightColor;
    // diffuse
    //vec3 lightDir = normalize(lightPos - FragPos);
    //lightDir *= -1;

    float diff = max(dot(normalize(sunDir), normal), 0.0);
    vec3 diffuse = diff * lightColor;

    // calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);       
    
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse)) * color;
    //vec3 lighting = vec3(shadow);

    //vec3 lighting = (ambient + (1.0 - shadow) * 1.0) * color;
    FragColor = vec4(lighting, 1.0);

    //FragColor = vec4((Normal + 1.0) * 0.5,1.0);
    //float depthValue = texture(shadowMap, TexCoords).r;
    //FragColor = vec4(vec3(depthValue), 1.0); 
    //FragColor = vec4(color,1.0);
}