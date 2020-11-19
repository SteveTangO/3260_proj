#version 330 core

out vec4 daColor;
in vec3 normalWorld;
in vec3 vertexPositionWorld;
in vec2 UV;
in vec4 shadowCoord;

uniform vec3 lightPositionWorld;
uniform vec3 eyePositionWorld;
uniform sampler2D myTextureSampler0;
uniform sampler2D shadowMap;
uniform int dir_light_parameter;

float ShadowCalculation(vec4 shadowCoord)
{

    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    vec3 normal = normalize(normalWorld);
    vec3 lightDir = normalize(lightPositionWorld - vertexPositionWorld);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}

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
    
    float shadow = ShadowCalculation(shadowCoord);
    
    vec3 theColor = ((1-shadow) * dir_light_parameter * (diffuse + specular) + ambient) * color;
    
    daColor = vec4(theColor, 1.0f);
}
