#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// Texturas
uniform sampler2D texture_diffuse1;
uniform sampler2D emissionMap;
uniform float emissionIntensity;

// Spotlight
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform float cutOff;
uniform float outerCutOff;

void main()
{
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;
    
    // ====== Luz ambiente, difusa y especular (Spotlight) ======
    vec3 lightColor = vec3(1.0);
    vec3 ambient = 0.1 * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDirVec = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDirVec), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirVec, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = lightColor * spec * 0.5;

    // Intensidad del spotlight
    float theta = dot(lightDirVec, normalize(-lightDir));
    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    diffuse *= intensity;
    specular *= intensity;

    // ====== Emisión adicional (glow) ======
    vec3 emission = texture(emissionMap, TexCoords).rgb;
    float emissionFactor = step(0.1, length(emission)); // Activar solo si hay brillo
    emission = emission * emissionIntensity * emissionFactor;

    // Color final con iluminación + emisión
    vec3 result = (ambient + diffuse + specular) * texColor + emission;

    FragColor = vec4(result, 1.0);
}