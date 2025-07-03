/*
 * shaders.c - Shader Management Implementation
 */

#include "shaders.h"
#include <math.h>

// Global shader programs
ShaderProgram shader_programs[SHADER_COUNT];

// Tessellation vertex shader
const char* tessellation_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec3 normal;\n"
"layout(location = 2) in vec2 texCoord;\n"
"\n"
"out vec3 vPosition;\n"
"out vec3 vNormal;\n"
"out vec2 vTexCoord;\n"
"\n"
"void main() {\n"
"    vPosition = position;\n"
"    vNormal = normal;\n"
"    vTexCoord = texCoord;\n"
"}\n";

// Tessellation control shader for adaptive LOD
const char* tessellation_tcs_shader =
"#version 410 core\n"
"layout(vertices = 4) out;\n"
"\n"
"in vec3 vPosition[];\n"
"in vec3 vNormal[];\n"
"in vec2 vTexCoord[];\n"
"\n"
"out vec3 tcPosition[];\n"
"out vec3 tcNormal[];\n"
"out vec2 tcTexCoord[];\n"
"\n"
"uniform mat4 view;\n"
"uniform vec3 viewPos;\n"
"\n"
"float getTessLevel(float distance) {\n"
"    float minDist = 2.0;\n"
"    float maxDist = 50.0;\n"
"    float maxTess = 64.0;\n"
"    float minTess = 4.0;\n"
"    \n"
"    float factor = clamp((maxDist - distance) / (maxDist - minDist), 0.0, 1.0);\n"
"    return mix(minTess, maxTess, factor);\n"
"}\n"
"\n"
"void main() {\n"
"    tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];\n"
"    tcNormal[gl_InvocationID] = vNormal[gl_InvocationID];\n"
"    tcTexCoord[gl_InvocationID] = vTexCoord[gl_InvocationID];\n"
"    \n"
"    if (gl_InvocationID == 0) {\n"
"        // Calculate distance-based tessellation levels\n"
"        vec3 center = (vPosition[0] + vPosition[1] + vPosition[2] + vPosition[3]) * 0.25;\n"
"        float distance = length(viewPos - center);\n"
"        float tessLevel = getTessLevel(distance);\n"
"        \n"
"        gl_TessLevelOuter[0] = tessLevel;\n"
"        gl_TessLevelOuter[1] = tessLevel;\n"
"        gl_TessLevelOuter[2] = tessLevel;\n"
"        gl_TessLevelOuter[3] = tessLevel;\n"
"        gl_TessLevelInner[0] = tessLevel;\n"
"        gl_TessLevelInner[1] = tessLevel;\n"
"    }\n"
"}\n";

// Tessellation evaluation shader with displacement
const char* tessellation_tes_shader =
"#version 410 core\n"
"layout(quads, equal_spacing, ccw) in;\n"
"\n"
"in vec3 tcPosition[];\n"
"in vec3 tcNormal[];\n"
"in vec2 tcTexCoord[];\n"
"\n"
"out vec3 tePosition;\n"
"out vec3 teNormal;\n"
"out vec2 teTexCoord;\n"
"out vec3 teTangent;\n"
"out vec3 teBitangent;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform sampler2D heightMap;\n"
"uniform sampler2D normalMap;\n"
"uniform float displacementScale;\n"
"uniform float time;\n"
"\n"
"// Perlin noise function for detail\n"
"vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }\n"
"\n"
"float snoise(vec2 v) {\n"
"    const vec4 C = vec4(0.211324865405187, 0.366025403784439,\n"
"                       -0.577350269189626, 0.024390243902439);\n"
"    vec2 i  = floor(v + dot(v, C.yy));\n"
"    vec2 x0 = v - i + dot(i, C.xx);\n"
"    vec2 i1;\n"
"    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
"    vec4 x12 = x0.xyxy + C.xxzz;\n"
"    x12.xy -= i1;\n"
"    i = mod289(i);\n"
"    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0))\n"
"                   + i.x + vec3(0.0, i1.x, 1.0));\n"
"    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);\n"
"    m = m*m;\n"
"    m = m*m;\n"
"    vec3 x = 2.0 * fract(p * C.www) - 1.0;\n"
"    vec3 h = abs(x) - 0.5;\n"
"    vec3 ox = floor(x + 0.5);\n"
"    vec3 a0 = x - ox;\n"
"    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0 + h*h);\n"
"    vec3 g;\n"
"    g.x  = a0.x  * x0.x  + h.x  * x0.y;\n"
"    g.yz = a0.yz * x12.xz + h.yz * x12.yw;\n"
"    return 130.0 * dot(m, g);\n"
"}\n"
"\n"
"float fractalNoise(vec2 uv, int octaves) {\n"
"    float value = 0.0;\n"
"    float amplitude = 0.5;\n"
"    for (int i = 0; i < octaves; i++) {\n"
"        value += amplitude * snoise(uv);\n"
"        uv *= 2.0;\n"
"        amplitude *= 0.5;\n"
"    }\n"
"    return value;\n"
"}\n"
"\n"
"void main() {\n"
"    // Bilinear interpolation\n"
"    vec3 p0 = mix(tcPosition[0], tcPosition[1], gl_TessCoord.x);\n"
"    vec3 p1 = mix(tcPosition[3], tcPosition[2], gl_TessCoord.x);\n"
"    vec3 position = mix(p0, p1, gl_TessCoord.y);\n"
"    \n"
"    vec3 n0 = mix(tcNormal[0], tcNormal[1], gl_TessCoord.x);\n"
"    vec3 n1 = mix(tcNormal[3], tcNormal[2], gl_TessCoord.x);\n"
"    vec3 normal = normalize(mix(n0, n1, gl_TessCoord.y));\n"
"    \n"
"    vec2 t0 = mix(tcTexCoord[0], tcTexCoord[1], gl_TessCoord.x);\n"
"    vec2 t1 = mix(tcTexCoord[3], tcTexCoord[2], gl_TessCoord.x);\n"
"    vec2 texCoord = mix(t0, t1, gl_TessCoord.y);\n"
"    \n"
"    // Sample height map and add procedural detail\n"
"    float height = texture(heightMap, texCoord).r;\n"
"    float detail = fractalNoise(texCoord * 20.0 + vec2(time * 0.01), 4) * 0.1;\n"
"    float displacement = (height + detail) * displacementScale;\n"
"    \n"
"    // Apply displacement\n"
"    position += normal * displacement;\n"
"    \n"
"    // Calculate tangent space for normal mapping\n"
"    vec3 edge1 = tcPosition[1] - tcPosition[0];\n"
"    vec3 edge2 = tcPosition[3] - tcPosition[0];\n"
"    vec2 deltaUV1 = tcTexCoord[1] - tcTexCoord[0];\n"
"    vec2 deltaUV2 = tcTexCoord[3] - tcTexCoord[0];\n"
"    \n"
"    float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);\n"
"    teTangent = normalize(f * (deltaUV2.y * edge1 - deltaUV1.y * edge2));\n"
"    teBitangent = normalize(f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));\n"
"    \n"
"    // Output\n"
"    tePosition = (model * vec4(position, 1.0)).xyz;\n"
"    teNormal = normalize(mat3(transpose(inverse(model))) * normal);\n"
"    teTexCoord = texCoord;\n"
"    \n"
"    gl_Position = projection * view * vec4(tePosition, 1.0);\n"
"}\n";

// Advanced fragment shader with PBR lighting
const char* tessellation_fragment_shader =
"#version 410 core\n"
"in vec3 tePosition;\n"
"in vec3 teNormal;\n"
"in vec2 teTexCoord;\n"
"in vec3 teTangent;\n"
"in vec3 teBitangent;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"uniform vec3 viewPos;\n"
"uniform float time;\n"
"\n"
"// Material properties\n"
"uniform sampler2D diffuseMap;\n"
"uniform sampler2D normalMap;\n"
"uniform sampler2D roughnessMap;\n"
"uniform sampler2D aoMap;\n"
"uniform sampler2D emissiveMap;\n"
"\n"
"// Lights\n"
"#define MAX_LIGHTS 16\n"
"uniform int numLights;\n"
"uniform vec3 lightPositions[MAX_LIGHTS];\n"
"uniform vec3 lightColors[MAX_LIGHTS];\n"
"uniform float lightIntensities[MAX_LIGHTS];\n"
"\n"
"// Shadow mapping\n"
"uniform sampler2D shadowMap;\n"
"uniform mat4 lightSpaceMatrix;\n"
"uniform int shadowsEnabled;\n"
"\n"
"// Fog\n"
"uniform vec3 fogColor;\n"
"uniform float fogDensity;\n"
"\n"
"// PBR calculations\n"
"vec3 getNormalFromMap() {\n"
"    vec3 tangentNormal = texture(normalMap, teTexCoord).xyz * 2.0 - 1.0;\n"
"    \n"
"    vec3 N = normalize(teNormal);\n"
"    vec3 T = normalize(teTangent - dot(teTangent, N) * N);\n"
"    vec3 B = cross(N, T);\n"
"    \n"
"    mat3 TBN = mat3(T, B, N);\n"
"    return normalize(TBN * tangentNormal);\n"
"}\n"
"\n"
"float DistributionGGX(vec3 N, vec3 H, float roughness) {\n"
"    float a = roughness * roughness;\n"
"    float a2 = a * a;\n"
"    float NdotH = max(dot(N, H), 0.0);\n"
"    float NdotH2 = NdotH * NdotH;\n"
"    \n"
"    float num = a2;\n"
"    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
"    denom = 3.14159265359 * denom * denom;\n"
"    \n"
"    return num / denom;\n"
"}\n"
"\n"
"float GeometrySchlickGGX(float NdotV, float roughness) {\n"
"    float r = (roughness + 1.0);\n"
"    float k = (r * r) / 8.0;\n"
"    \n"
"    float num = NdotV;\n"
"    float denom = NdotV * (1.0 - k) + k;\n"
"    \n"
"    return num / denom;\n"
"}\n"
"\n"
"float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
"    float NdotV = max(dot(N, V), 0.0);\n"
"    float NdotL = max(dot(N, L), 0.0);\n"
"    float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
"    float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n"
"    \n"
"    return ggx1 * ggx2;\n"
"}\n"
"\n"
"vec3 fresnelSchlick(float cosTheta, vec3 F0) {\n"
"    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);\n"
"}\n"
"\n"
"float ShadowCalculation(vec4 fragPosLightSpace) {\n"
"    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;\n"
"    projCoords = projCoords * 0.5 + 0.5;\n"
"    \n"
"    float closestDepth = texture(shadowMap, projCoords.xy).r;\n"
"    float currentDepth = projCoords.z;\n"
"    \n"
"    float bias = 0.005;\n"
"    float shadow = 0.0;\n"
"    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);\n"
"    \n"
"    // PCF filtering\n"
"    for(int x = -1; x <= 1; ++x) {\n"
"        for(int y = -1; y <= 1; ++y) {\n"
"            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;\n"
"            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;\n"
"        }\n"
"    }\n"
"    shadow /= 9.0;\n"
"    \n"
"    if(projCoords.z > 1.0)\n"
"        shadow = 0.0;\n"
"    \n"
"    return shadow;\n"
"}\n"
"\n"
"void main() {\n"
"    vec3 albedo = pow(texture(diffuseMap, teTexCoord).rgb, vec3(2.2));\n"
"    vec3 normal = getNormalFromMap();\n"
"    float roughness = texture(roughnessMap, teTexCoord).r;\n"
"    float ao = texture(aoMap, teTexCoord).r;\n"
"    vec3 emissive = texture(emissiveMap, teTexCoord).rgb;\n"
"    \n"
"    vec3 N = normalize(normal);\n"
"    vec3 V = normalize(viewPos - tePosition);\n"
"    \n"
"    vec3 F0 = vec3(0.04);\n"
"    F0 = mix(F0, albedo, 0.0); // metallic = 0 for rocks\n"
"    \n"
"    vec3 Lo = vec3(0.0);\n"
"    \n"
"    // Calculate lighting contribution from each light\n"
"    for(int i = 0; i < numLights && i < MAX_LIGHTS; ++i) {\n"
"        vec3 L = normalize(lightPositions[i] - tePosition);\n"
"        vec3 H = normalize(V + L);\n"
"        float distance = length(lightPositions[i] - tePosition);\n"
"        float attenuation = 1.0 / (distance * distance);\n"
"        vec3 radiance = lightColors[i] * lightIntensities[i] * attenuation;\n"
"        \n"
"        float NDF = DistributionGGX(N, H, roughness);\n"
"        float G = GeometrySmith(N, V, L, roughness);\n"
"        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);\n"
"        \n"
"        vec3 kS = F;\n"
"        vec3 kD = vec3(1.0) - kS;\n"
"        \n"
"        vec3 numerator = NDF * G * F;\n"
"        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;\n"
"        vec3 specular = numerator / denominator;\n"
"        \n"
"        float NdotL = max(dot(N, L), 0.0);\n"
"        Lo += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;\n"
"    }\n"
"    \n"
"    // Shadow calculation\n"
"    float shadow = 0.0;\n"
"    if (shadowsEnabled > 0) {\n"
"        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(tePosition, 1.0);\n"
"        shadow = ShadowCalculation(fragPosLightSpace);\n"
"    }\n"
"    \n"
"    vec3 ambient = vec3(0.03) * albedo * ao;\n"
"    vec3 color = ambient + (1.0 - shadow) * Lo + emissive;\n"
"    \n"
"    // Fog\n"
"    float dist = length(viewPos - tePosition);\n"
"    float fogFactor = 1.0 - exp(-fogDensity * dist);\n"
"    color = mix(color, fogColor, fogFactor);\n"
"    \n"
"    // Tone mapping and gamma correction\n"
"    color = color / (color + vec3(1.0));\n"
"    color = pow(color, vec3(1.0/2.2));\n"
"    \n"
"    FragColor = vec4(color, 1.0);\n"
"}\n";

// Shadow mapping shaders
const char* shadow_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec3 position;\n"
"\n"
"uniform mat4 lightSpaceMatrix;\n"
"uniform mat4 model;\n"
"\n"
"void main() {\n"
"    gl_Position = lightSpaceMatrix * model * vec4(position, 1.0);\n"
"}\n";

const char* shadow_fragment_shader =
"#version 410 core\n"
"\n"
"void main() {\n"
"    // gl_FragDepth is automatically written\n"
"}\n";

// Crystal shader for glowing crystals
const char* crystal_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec3 normal;\n"
"\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"void main() {\n"
"    FragPos = vec3(model * vec4(position, 1.0));\n"
"    Normal = mat3(transpose(inverse(model))) * normal;\n"
"    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"}\n";

const char* crystal_fragment_shader =
"#version 410 core\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"uniform vec3 viewPos;\n"
"uniform vec3 crystalColor;\n"
"uniform float time;\n"
"\n"
"void main() {\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    \n"
"    // Fresnel effect for rim lighting\n"
"    float fresnel = pow(1.0 - dot(viewDir, norm), 2.0);\n"
"    \n"
"    // Animated glow\n"
"    float glow = sin(time * 2.0) * 0.5 + 0.5;\n"
"    \n"
"    vec3 color = crystalColor * (0.3 + fresnel * 0.7);\n"
"    color += crystalColor * glow * 0.5;\n"
"    \n"
"    FragColor = vec4(color, 0.8);\n"
"}\n";

// Water shader
const char* water_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec3 position;\n"
"layout(location = 1) in vec2 texCoord;\n"
"\n"
"out vec3 FragPos;\n"
"out vec2 TexCoord;\n"
"out vec4 ClipSpaceCoords;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float time;\n"
"\n"
"void main() {\n"
"    vec3 pos = position;\n"
"    \n"
"    // Simple wave animation\n"
"    pos.y += sin(pos.x * 4.0 + time) * 0.1;\n"
"    pos.y += cos(pos.z * 3.0 + time * 1.5) * 0.08;\n"
"    \n"
"    FragPos = vec3(model * vec4(pos, 1.0));\n"
"    TexCoord = texCoord;\n"
"    ClipSpaceCoords = projection * view * vec4(FragPos, 1.0);\n"
"    gl_Position = ClipSpaceCoords;\n"
"}\n";

const char* water_fragment_shader =
"#version 410 core\n"
"in vec3 FragPos;\n"
"in vec2 TexCoord;\n"
"in vec4 ClipSpaceCoords;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"uniform sampler2D reflectionTexture;\n"
"uniform sampler2D refractionTexture;\n"
"uniform sampler2D dudvMap;\n"
"uniform sampler2D normalMap;\n"
"uniform sampler2D depthMap;\n"
"\n"
"uniform vec3 viewPos;\n"
"uniform float time;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"\n"
"const float waveStrength = 0.02;\n"
"const float shineDamper = 20.0;\n"
"const float reflectivity = 0.6;\n"
"\n"
"void main() {\n"
"    vec2 ndc = (ClipSpaceCoords.xy / ClipSpaceCoords.w) / 2.0 + 0.5;\n"
"    vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);\n"
"    vec2 refractTexCoords = vec2(ndc.x, ndc.y);\n"
"    \n"
"    // Water depth\n"
"    float depth = texture(depthMap, refractTexCoords).r;\n"
"    float floorDistance = 2.0 * 0.1 * 100.0 / (100.0 + 0.1 - (2.0 * depth - 1.0) * (100.0 - 0.1));\n"
"    depth = gl_FragCoord.z;\n"
"    float waterDistance = 2.0 * 0.1 * 100.0 / (100.0 + 0.1 - (2.0 * depth - 1.0) * (100.0 - 0.1));\n"
"    float waterDepth = floorDistance - waterDistance;\n"
"    \n"
"    // Distortion\n"
"    vec2 distortedTexCoords = texture(dudvMap, vec2(TexCoord.x + time * 0.03, TexCoord.y)).rg * 0.1;\n"
"    distortedTexCoords = TexCoord + vec2(distortedTexCoords.x, distortedTexCoords.y + time * 0.03);\n"
"    vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength;\n"
"    \n"
"    reflectTexCoords += totalDistortion;\n"
"    reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);\n"
"    reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, -0.001);\n"
"    \n"
"    refractTexCoords += totalDistortion;\n"
"    refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);\n"
"    \n"
"    vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);\n"
"    vec4 refractColor = texture(refractionTexture, refractTexCoords);\n"
"    \n"
"    // Normal mapping\n"
"    vec4 normalMapColor = texture(normalMap, distortedTexCoords);\n"
"    vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b, normalMapColor.g * 2.0 - 1.0);\n"
"    normal = normalize(normal);\n"
"    \n"
"    // Fresnel\n"
"    vec3 viewVector = normalize(viewPos - FragPos);\n"
"    float refractiveFactor = dot(viewVector, normal);\n"
"    refractiveFactor = pow(refractiveFactor, 0.5);\n"
"    \n"
"    // Specular highlights\n"
"    vec3 reflectedLight = reflect(normalize(FragPos - lightPos), normal);\n"
"    float specular = max(dot(reflectedLight, viewVector), 0.0);\n"
"    specular = pow(specular, shineDamper);\n"
"    vec3 specularHighlights = lightColor * specular * reflectivity;\n"
"    \n"
"    vec4 finalColor = mix(reflectColor, refractColor, refractiveFactor);\n"
"    finalColor = mix(finalColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + vec4(specularHighlights, 0.0);\n"
"    finalColor.a = clamp(waterDepth / 5.0, 0.0, 1.0);\n"
"    \n"
"    FragColor = finalColor;\n"
"}\n";

// Shader compilation and linking functions
int check_shader_compile_status(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        char* log = (char*)malloc(log_length);
        glGetShaderInfoLog(shader, log_length, &log_length, log);
        fprintf(stderr, "Shader compilation failed:\n%s\n", log);
        free(log);
        return 0;
    }
    return 1;
}

int check_program_link_status(GLuint program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        char* log = (char*)malloc(log_length);
        glGetProgramInfoLog(program, log_length, &log_length, log);
        fprintf(stderr, "Program linking failed:\n%s\n", log);
        free(log);
        return 0;
    }
    return 1;
}

GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    if (!check_shader_compile_status(shader)) {
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint create_shader_program(const char* vertex_source, const char* fragment_source) {
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (!vertex_shader) return 0;
    
    GLuint fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (!check_program_link_status(program)) {
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

GLuint create_tessellation_program(const char* vertex_source, const char* tcs_source,
                                  const char* tes_source, const char* fragment_source) {
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (!vertex_shader) return 0;
    
    GLuint tcs_shader = compile_shader(tcs_source, GL_TESS_CONTROL_SHADER);
    if (!tcs_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }
    
    GLuint tes_shader = compile_shader(tes_source, GL_TESS_EVALUATION_SHADER);
    if (!tes_shader) {
        glDeleteShader(vertex_shader);
        glDeleteShader(tcs_shader);
        return 0;
    }
    
    GLuint fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        glDeleteShader(tcs_shader);
        glDeleteShader(tes_shader);
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, tcs_shader);
    glAttachShader(program, tes_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(tcs_shader);
    glDeleteShader(tes_shader);
    glDeleteShader(fragment_shader);
    
    if (!check_program_link_status(program)) {
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

void init_shaders(void) {
    // Initialize tessellation shader
    shader_programs[SHADER_TESSELLATION].program = create_tessellation_program(
        tessellation_vertex_shader, tessellation_tcs_shader,
        tessellation_tes_shader, tessellation_fragment_shader
    );
    
    // Initialize shadow map shader
    shader_programs[SHADER_SHADOW_MAP].program = create_shader_program(
        shadow_vertex_shader, shadow_fragment_shader
    );
    
    // Initialize crystal shader
    shader_programs[SHADER_CRYSTAL].program = create_shader_program(
        crystal_vertex_shader, crystal_fragment_shader
    );
    
    // Initialize water shader
    shader_programs[SHADER_WATER].program = create_shader_program(
        water_vertex_shader, water_fragment_shader
    );
    
    // Get uniform locations for tessellation shader
    ShaderProgram* tess = &shader_programs[SHADER_TESSELLATION];
    tess->model_loc = glGetUniformLocation(tess->program, "model");
    tess->view_loc = glGetUniformLocation(tess->program, "view");
    tess->projection_loc = glGetUniformLocation(tess->program, "projection");
    tess->view_pos_loc = glGetUniformLocation(tess->program, "viewPos");
    tess->time_loc = glGetUniformLocation(tess->program, "time");
    tess->light_space_matrix_loc = glGetUniformLocation(tess->program, "lightSpaceMatrix");
}

void cleanup_shaders(void) {
    for (int i = 0; i < SHADER_COUNT; i++) {
        if (shader_programs[i].program) {
            glDeleteProgram(shader_programs[i].program);
        }
    }
}

void use_shader(ShaderType type) {
    glUseProgram(shader_programs[type].program);
}

void set_uniform_mat4(GLuint program, const char* name, const float* matrix) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
    }
}

void set_uniform_vec3(GLuint program, const char* name, float x, float y, float z) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
        glUniform3f(loc, x, y, z);
    }
}

void set_uniform_float(GLuint program, const char* name, float value) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
        glUniform1f(loc, value);
    }
}

void set_uniform_int(GLuint program, const char* name, int value) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc != -1) {
        glUniform1i(loc, value);
    }
}
