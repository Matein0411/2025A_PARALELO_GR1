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

void main()
{
    vec3 lightColor = vec3(1.0);
    vec3 ambient = 0.1 * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDirVec = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDirVec), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirVec, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = lightColor * spec * 0.5;

    // Spotlight intensity
    float theta = dot(lightDirVec, normalize(-lightDir));
    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    diffuse *= intensity;
    specular *= intensity;

    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;

    FragColor = vec4(result, texColor.a);
}
