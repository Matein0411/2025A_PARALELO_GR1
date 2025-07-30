#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    // Como las coordenadas ya están en el rango -1 a 1, las pasamos directamente.
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
