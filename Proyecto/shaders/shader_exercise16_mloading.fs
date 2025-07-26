#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;

// Spotlight
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform float cutOff;
uniform float outerCutOff;

// Point lights
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);

    // ---- Point Lights ----
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        vec3 lightDirVec = normalize(pointLights[i].position - FragPos);
        float diff = max(dot(norm, lightDirVec), 0.0);
        vec3 diffuse = diff * pointLights[i].color;

        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));

        vec3 ambient = 0.15 * pointLights[i].color;
        ambient *= attenuation;
        diffuse *= attenuation;

        result += ambient + diffuse;
    }

    // ---- Spotlight ----
    vec3 lightDirVec = normalize(lightPos - FragPos);
    float theta = dot(lightDirVec, normalize(-lightDir));
    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(norm, lightDirVec), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    vec3 reflectDir = reflect(-lightDirVec, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(1.0) * spec * 0.5;

    vec3 ambient = 0.1 * vec3(1.0);
    diffuse *= intensity;
    specular *= intensity;

    result += ambient + diffuse + specular;

    // Final color
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    FragColor = vec4(result * texColor.rgb, texColor.a);
}
