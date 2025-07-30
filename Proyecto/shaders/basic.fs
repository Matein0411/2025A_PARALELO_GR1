#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// La textura que le vamos a pasar desde C++
uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoord);
}
