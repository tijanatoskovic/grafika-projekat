#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentPointLightPos;
    vec3 TangentSpotLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform SpotLight spotLight;
uniform PointLight pointLight;
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = aPos;
    vs_out.TexCoords = aTexCoords;

     mat3 normalMatrix = transpose(inverse(mat3(model)));
     vec3 T = normalize(mat3(model) * aTangent);
     vec3 B = normalize(mat3(model) * aBitangent);
     vec3 N = normalize(mat3(model) * aNormal);

     mat3 TBN = transpose(mat3(T, B, N));

     vs_out.TangentPointLightPos = TBN * pointLight.position;
     vs_out.TangentSpotLightPos = TBN * spotLight.position;

     vs_out.TangentViewPos  = TBN * viewPos;
     vs_out.TangentFragPos  = TBN * vs_out.FragPos;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}