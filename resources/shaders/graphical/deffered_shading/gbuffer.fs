#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 sun_direction;
uniform float fog_density;
uniform vec3 player_camera_position;
uniform mat4 player_camera_view_matrix;

void main()
{
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;

    vec4 worldPos = inverse(player_camera_view_matrix) * vec4(FragPos, 1.0);
    vec3 FragPosWorld = worldPos.xyz / worldPos.w; // Perspective divide

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Alpha = texture(gAlbedoSpec, TexCoords).a;

    if (Alpha < 0.1) discard;

    if (FragPos.z == 1) {
        FragColor = vec4(Albedo, 1.0);
        return;
    }

    vec3 lightColor = vec3(1, 0.984, 0) / 4;

    float diff = max(dot(Normal, sun_direction), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(player_camera_position - FragPosWorld);
    vec3 reflectDir = reflect(sun_direction, Normal);

    float specularStrength = 0.5;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 ambient = vec3(0.4) + dot(sun_direction, vec3(0, 1, 0));

    vec3 result = (ambient + diffuse + specular) * Albedo;

    const vec3 fogColor = vec3(0.529, 0.808, 0.922) - 0.1;

    float distance = length(player_camera_position - FragPosWorld);

    // Exponential fog factor
    float fogFactor = exp(-fog_density * distance);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Blend between object color and fog color
    vec3 finalColor = mix(fogColor, result.rgb, fogFactor);

    FragColor = vec4(finalColor, Alpha);
}
