#version 330 core

out vec4 daColor;
in vec3 normalWorld;
in vec3 vertexPositionWorld;
in vec2 UV;

uniform vec3 lightPositionWorld;
uniform vec3 eyePositionWorld;
uniform sampler2D myTextureSampler0;

uniform int dir_light_parameter;

void main()
{
    
    
    
    vec3 color = texture(myTextureSampler0, UV).rgb;
    vec3 normal = normalize(normalWorld);
    vec3 lightColor = vec3(0.3);
    
    vec3 ambient = 0.6 * color;
    
    vec3 lightDir = normalize(lightPositionWorld - vertexPositionWorld);
    
    float dotLightNormal = dot(lightDir, normal);
    float diff = max(dotLightNormal, 0.0f);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(eyePositionWorld - vertexPositionWorld);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), 64.0);
    vec3 specular = spec * lightColor;
    
    vec3 theColor = (dir_light_parameter * (diffuse + specular) + ambient) * color;
    
    daColor = vec4(theColor, 1.0f);
}

