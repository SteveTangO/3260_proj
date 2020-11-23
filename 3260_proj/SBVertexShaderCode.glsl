#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 projectionMatrix;

out vec3 texcoords;

void main()
{
    texcoords = position;
    gl_Position = projectionMatrix * vec4(position, 1.0f);
}
