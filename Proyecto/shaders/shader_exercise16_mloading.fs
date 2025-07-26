#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform float lightIntensity;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    vec3 ambientColor;
    if (FragPos.x < -10.0)
        ambientColor = vec3(0.8, 0.1, 0.1); // rojo
    else if (FragPos.x > 10.0)
        ambientColor = vec3(0.2, 0.5, 0.8); // azul
    else
        ambientColor = vec3(0.43, 0.5, 0.5); // AuroMetalSaurus

    FragColor = texColor * vec4(ambientColor * lightIntensity, 1.0);
}
