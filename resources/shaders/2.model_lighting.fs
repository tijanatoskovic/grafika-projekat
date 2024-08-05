#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
    float shininessBP;  // Blinn-Phong shininess
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight pointLight;
uniform Material material;
uniform vec3 viewPosition;
uniform bool blinn;
uniform DirLight dirLight;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    float spec = 0.0;
    if(blinn) {
        // halfway direction vector for Blinn-Phong
        vec3 halfDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfDir), 0.0), material.shininessBP);
    } else {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // ambient
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));

    // diffuse component
    vec4 diffSample = texture(material.texture_diffuse1, TexCoords);
    if(diffSample.a < 0.4) {
        discard;
    }
    vec3 diffuse = light.diffuse * diff * vec3(diffSample);

    // specular
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    //ambient
    vec3 ambient = light.ambient * texture(material.texture_diffuse1, TexCoords).rgb;
    //diffuse
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.texture_diffuse1, TexCoords).rgb;
    //specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0f;

    // Blinn-Phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir),0.0), material.shininess);

    vec3 specular = light.specular * spec * texture(material.texture_specular1, TexCoords).rgb;

    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcPointLight(pointLight, normal, FragPos, viewDir);
    result += CalcDirLight(dirLight, normal, viewDir);
    FragColor = vec4(result, 1.0);
}