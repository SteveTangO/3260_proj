#version 330 core

out vec4 daColor;
in vec3 normalWorld;
in vec3 vertexPositionWorld;
in vec2 UV;
in vec2 normalUV;

uniform vec3 lightPositionWorld1;
uniform vec3 lightPositionWorld2;
uniform vec3 eyePositionWorld;
uniform sampler2D myTextureSampler0;
uniform sampler2D myTextureSampler1;

uniform int dir_light_parameter;

void main()
{

    vec3 color = texture(myTextureSampler0, UV).rgb;
    vec3 normal = normalize(normalWorld);
    
    //range transform
    normal = texture(myTextureSampler1, normalUV).rgb;
    normal = normalize(normal * 2.0f - 1.0f);
    vec3 lightColor1 = vec3(0.3);
    vec3 lightColor2 = vec3(0.1, 0.4, 0.1);
    
    vec3 ambient = 0.6 * color;
    
    vec3 lightDir1 = normalize(lightPositionWorld1 - vertexPositionWorld);
    
    float dotLightNormal1 = dot(lightDir1, normal);
    float diff1 = max(dotLightNormal1, 0.0f);
    vec3 diffuse1 = diff1 * lightColor1;
    
    vec3 viewDir = normalize(eyePositionWorld - vertexPositionWorld);
    vec3 halfwayDir1 = normalize(lightDir1 + viewDir);
    float spec1 = pow(max(dot(normal, halfwayDir1), 0.0f), 64.0);
    vec3 specular1 = spec1 * lightColor1;
    
    vec3 lightDir2 = normalize(lightPositionWorld2 - vertexPositionWorld);
    
    float dotLightNormal2 = dot(lightDir2, normal);
    float diff2 = max(dotLightNormal2, 0.0f);
    vec3 diffuse2 = diff2 * lightColor2;
    
    vec3 halfwayDir2 = normalize(lightDir2 + viewDir);
    float spec2 = pow(max(dot(normal, halfwayDir2), 0.0f), 64.0);
    vec3 specular2 = spec2 * lightColor2;
    
    vec3 theColor = (dir_light_parameter * (diffuse1 + specular1) + dir_light_parameter * (diffuse2 + specular2) + ambient) * color;
    
    daColor = vec4(theColor, 1.0f);
}

