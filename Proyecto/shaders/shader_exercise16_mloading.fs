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

// === Nuevas luces tenues ===
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

#define NUM_POINT_LIGHTS 16
uniform PointLight pointLights[NUM_POINT_LIGHTS];

void main()
{
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;
    
    // ===== Luz ambiente, difusa y especular (Spotlight) =====
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

    // ===== Luz tenue (Point Lights) =====
    vec3 pointLightResult = vec3(0.0);
    for (int i = 0; i < NUM_POINT_LIGHTS; ++i) {
        vec3 toLight = normalize(pointLights[i].position - FragPos);
        float diffPL = max(dot(norm, toLight), 0.0);

        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);

        vec3 ambientPL = 0.05 * pointLights[i].color;
        vec3 diffusePL = diffPL * pointLights[i].color;

        ambientPL *= attenuation;
        diffusePL *= attenuation;

        pointLightResult += ambientPL + diffusePL;
    }

    // ===== Emisión adicional (glow) =====
    vec3 emission = texture(emissionMap, TexCoords).rgb;
    float emissionFactor = step(0.1, length(emission)); // Activar solo si hay brillo
    emission = emission * emissionIntensity * emissionFactor;

    // ===== Color final =====
    vec3 result = (ambient + diffuse + specular + pointLightResult) * texColor + emission;
    FragColor = vec4(result, 1.0);
}
