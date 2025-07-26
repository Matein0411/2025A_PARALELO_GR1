#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D emissionMap;
uniform float emissionIntensity;

void main()
{
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 emission = texture(emissionMap, TexCoords).rgb;
    
    // Aplicar emisión solo donde el color supera un umbral
    float emissionFactor = step(0.1, length(emission));
    emission = emission * emissionIntensity * emissionFactor;
    
    // Combinación final (aditiva para mejor efecto brillante)
    vec3 finalColor = baseColor + emission;
    
    FragColor = vec4(finalColor, 1.0);
}