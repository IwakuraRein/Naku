#version 450
// #define VULKAN 130

#define MAX_LIGHT_NUM 64

layout(location = 0) in vec3 T;
layout(location = 1) in vec3 B;
layout(location = 2) in vec3 N;
layout(location = 3) in vec4 worldPos;
layout(location = 4) in vec4 vertColor;
layout(location = 5) in vec2 uv;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormalOcclusion;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMetalRough;
layout(location = 4) out vec4 outEmission;
layout(location = 5) out uint outMtlId;
layout(location = 6) out uint outObjId;

// layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputNormalOcclusion;
// layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputAlbedo;
// layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputMetalRough;
// layout(input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputEmission;
// layout(input_attachment_index = 4, set = 1, binding = 4) uniform usubpassInput inputObjId;

layout(push_constant) uniform Push {
    vec4 albedo;
    vec4 emission;
    vec4 offsetTilling;
    float metalness;
    float roughness;
    float ior;
    int side;
    int alphaMode;
    int hasTexture;
    int mtlId;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo {
    float time;
    float tanFov;
    int height;
    int width;
    mat4 projectionView;
    mat4 projectionInv;
    mat4 viewInv;
    vec4 viewPort; // x, y, w, h
    vec4 clip; // min depth, max depth, near clip, far clip
    vec3 camPos;
    vec3 camDir;
    vec3 clearColor;
    vec4 environment;
} globalUbo;

layout(set = 0, binding = 1) uniform ModelUbo {
    mat4 transformMat;
    mat4 normalMat;
    mat4 rotMat;
    int objId;
} modelUbo;

struct LightInfo {
    vec3 position; // for all
    vec4 emission; // for all
    vec3 direction; // for spot and directional
    mat4 projViewMat;
    int type;
    int shadowmap;
    float contact_shadow;
    float radius; // for point and spot
    float outerAngle; // for spot
    float innerAngleRatio; // for spot
};
layout(set = 0, binding = 2) uniform LightUbo {
    int lightNum;
    LightInfo infos[MAX_LIGHT_NUM];
} lightUbo;

// layout(set = 1, binding = 0) uniform sampler2D inputColor;
// layout(set = 1, binding = 0) uniform sampler2D inputDepth;

layout(set = 1, binding = 0) uniform sampler2DArray normalShadowmaps;
layout(set = 1, binding = 1) uniform samplerCubeArray omniShadowmaps;

layout(set = 2, binding = 0) uniform sampler2D baseTex;
layout(set = 2, binding = 1) uniform sampler2D normalTex;
layout(set = 2, binding = 2) uniform sampler2D metalnessTex;
layout(set = 2, binding = 3) uniform sampler2D roughnessTex;
layout(set = 2, binding = 4) uniform sampler2D occlusionTex;
layout(set = 2, binding = 5) uniform sampler2D emissionTex;

float distZ(float z, float zNear, float zFar) {
    float z_n = 2.0 * z - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}
const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);

void main() {
    vec3 albedo;
    float alpha;
    vec3 diffuseLight = vec3(0.0, 0.0, 0.0);
    vec3 emission;
    if((push.hasTexture & (1 << 5)) != 0) {
        emission = texture(emissionTex, uv).rgb;
    } else
        emission = push.emission.xyz * push.emission.w;

    vec3 normal;
    mat3 TBN = mat3(T, B, N);
    if((push.hasTexture & (1 << 1)) != 0) {
        vec3 bump = normalize(texture(normalTex, uv).xyz);
        bump = texture(normalTex, uv).xyz * 2.0 - 1.0;
        bump.xy = bump.xy * -1;
        bump.z = sqrt(1.0 - clamp(dot(bump.xy, bump.xy), 0, 1));
        normal = normalize(TBN * bump);
    } else {
        normal = N;
    }
    if(push.side == 1)
        normal = -normal;
    if((push.hasTexture & (1 << 0)) != 0) {
        albedo = vertColor.rgb * texture(baseTex, uv).rgb;
        alpha = vertColor.a * texture(baseTex, uv).a;
    } else {
        albedo = vertColor.rgb;
        alpha = vertColor.a;
    }

    int shadowmapCount1 = 0;
    int shadowmapCount2 = 0;
    for(int i = 0; i < lightUbo.lightNum; i++) {
        vec3 directionToLight0 = lightUbo.infos[i].position.xyz - worldPos.xyz;
        vec3 directionToLight = normalize(directionToLight0);
        if(lightUbo.infos[i].shadowmap > 0) {
            float bias = 0;
            if(push.side == 1) {
                bias = 0.001;
            }
            if(lightUbo.infos[i].type == 1 || lightUbo.infos[i].type == 2) {
                vec4 shadowPos = biasMat * lightUbo.infos[i].projViewMat * worldPos;
                shadowPos /= shadowPos.w;
                if(shadowPos.z - bias > texture(normalShadowmaps, vec3(shadowPos.xy, shadowmapCount1++)).x)
                    continue;
            } else {
                if(length(directionToLight0) - bias > distZ(texture(omniShadowmaps, vec4(-directionToLight0, shadowmapCount2++)).x, 0.01, lightUbo.infos[i].radius))
                    continue;
            }
        }
        float attenuation = 1.0;
        if(lightUbo.infos[i].type == 0 || lightUbo.infos[i].type == 1) {
            float dist2 = length(directionToLight0);
            if(dist2 >= lightUbo.infos[i].radius * lightUbo.infos[i].radius)
                continue;
            attenuation = 1.0 / dist2; // distance squared
        }
        vec3 lightColor = lightUbo.infos[i].emission.xyz * lightUbo.infos[i].emission.w * attenuation;
        vec3 thisLight;
        if(lightUbo.infos[i].type == 0 || lightUbo.infos[i].type == 1)
            thisLight = lightColor * max(dot(normal, directionToLight), 0);
        else
            thisLight = lightColor * max(dot(normal, -lightUbo.infos[i].direction), 0);
        if(lightUbo.infos[i].type == 1) {
            float cosine = dot(directionToLight, -lightUbo.infos[i].direction);
            float outerAngle = radians(lightUbo.infos[i].outerAngle) / 2;
            float innerCos = cos(lightUbo.infos[i].innerAngleRatio * outerAngle);
            float outerCos = cos(outerAngle);
            if(cosine < innerCos) {
                thisLight /= max((outerCos - innerCos) / (outerCos - cosine), 0);
            }
            if(cosine < outerCos)
                thisLight *= 0;
        }
        diffuseLight += thisLight;
    }
    diffuseLight += (globalUbo.environment.xyz * globalUbo.environment.w);
    outColor = vec4(diffuseLight * albedo + emission.xyz, alpha);

    outNormalOcclusion.xyz = normal;
    outAlbedo = vec4(albedo, alpha);
    outEmission = vec4(emission, alpha);
    if((push.hasTexture & (1 << 2)) != 0)
        outMetalRough.x = texture(metalnessTex, uv).x;
    else
        outMetalRough.x = push.metalness;
    if((push.hasTexture & (1 << 3)) != 0)
        outMetalRough.y = texture(roughnessTex, uv).x;
    else
        outMetalRough.y = push.roughness;
    if((push.hasTexture & (1 << 4)) != 0) {
        outNormalOcclusion.w = texture(occlusionTex, uv).r;
    } else
        outNormalOcclusion.w = 0.0;
    outObjId = modelUbo.objId;
    outMtlId = push.mtlId;
}