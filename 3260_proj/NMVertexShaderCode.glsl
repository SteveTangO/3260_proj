#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 normal;

uniform mat4 modelTransformMatrix;
uniform mat4 projectionMatrix;

out vec2 UV;
out vec3 normalWorld;
out vec3 vertexPositionWorld;

void main()
{
    vec4 v = vec4(position, 1.0f);
    vec4 newPosition = modelTransformMatrix * v;
    gl_Position = projectionMatrix * newPosition;
    
    vec4 normalTemp = modelTransformMatrix * vec4(normal, 0);
    normalWorld = normalTemp.xyz;
    
    vertexPositionWorld = newPosition.xyz;
    
    UV = vertexUV;
}
