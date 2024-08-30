#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 sunDir;
uniform vec3 camDir;
uniform vec3 camPos;

float angV3(vec3 v1, vec3 v2) {
    return acos(dot(normalize(v1), normalize(v2)));
}

void main()
{    
    vec3 rp = normalize(TexCoords); //relative position

    float angSun = angV3(rp,sunDir);

    vec4 finalColor = vec4(0.529, 0.808, 0.922, 1.0) - 0.1;
    finalColor = mix(vec4(0.8,0.8,0.8,1), finalColor, (TexCoords.y + 1) * 0.5);

    if (angSun < 0.03){
        finalColor = vec4(vec3(1), 1.0); // white
    } else if (angSun < 0.1) {
        float intensity = smoothstep(0.03, 0.1, angSun);
        finalColor = mix(vec4(1.0, 0.8, 0.6,1.0), finalColor, intensity); // Sun halo
    }

    FragColor = finalColor;
    //FragColor = texture(skybox, TexCoords);
}