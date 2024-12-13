#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D AmbientOcclusion;

uniform vec3 sun_direction;
uniform vec3 player_camera_position;

void main()
{             
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    float occlusion = texture(AmbientOcclusion, TexCoords).r;

    if(FragPos.z == 1){ // Avoid the skybox
        FragColor = vec4(Albedo, 1.0);
        return;
    }

    FragColor = vec4(vec3(1) * occlusion, 1.0);
}  