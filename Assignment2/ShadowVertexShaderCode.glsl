#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelTransformMatrix;

void main(){
    vec4 v = vec4(position, 1.0f);
    gl_Position =  lightSpaceMatrix * modelTransformMatrix * v;
}

