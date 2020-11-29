#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 normal;

uniform mat4 modelTransformMatrix;
uniform mat4 projectionMatrix;


out vec2 UV;
out vec3 normalWorld;
out vec3 vertexPositionWorld;

uniform sampler2D myTextureSampler1;

//uniform sampler2D myTextureSampler_1;

void main()
{
    
    vec3 coloradasadasda = texture(myTextureSampler1, UV).rgb;
    vec4 v = vec4(position, 1.0f);
    vec4 newPosition = modelTransformMatrix * v;
    gl_Position = projectionMatrix * newPosition;
    
    UV = vertexUV;
    
    vec4 normalTemp = modelTransformMatrix * vec4(normal, 0);
    normalWorld = normalTemp.xyz;
    
    vertexPositionWorld = newPosition.xyz;
    
    
}

