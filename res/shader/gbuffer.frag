#version 450
// #define VULKAN 130

#define MAX_POINT_LIGHT_NUM 16
#define MAX_SPOT_LIGHT_NUM 16
#define MAX_DIRECTIONAL_LIGHT_NUM 16

layout(location = 0) in vec3 T;
layout(location = 1) in vec3 B;
layout(location = 2) in vec3 N;
layout(location = 3) in vec4 worldPos;
layout(location = 4) in vec3 vertColor;
layout(location = 5) in vec2 uv;

layout(location = 0) out vec4 outNormalOcclusion;
layout(location = 1) out vec3 outAlbedo;
layout(location = 2) out vec4 outMetalRough;
layout(location = 3) out vec4 outEmission;
layout(location = 4) out uint outMtlId;
layout(location = 5) out uint outObjId;

layout(push_constant) uniform Push {
    vec4 albedo;
    vec4 emission;
    vec4 offsetTilling;
    float metalness;
    float roughness;
    float ior;
    int doubleSided;
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
    vec4 camPos;
    vec4 camDir;
} globalUbo;

layout(set = 0, binding = 1) uniform ModelUbo {
    mat4 transformMat;
    mat4 normalMat;
    mat4 rotMat;
    int objId;
} modelUbo;

layout(set = 1, binding = 0) uniform sampler2D baseTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform sampler2D metalnessTex;
layout(set = 1, binding = 3) uniform sampler2D roughnessTex;
layout(set = 1, binding = 4) uniform sampler2D occlusionTex;
layout(set = 1, binding = 5) uniform sampler2D emissionTex;
// layout(set = 1, binding = 6) uniform sampler2D iorTex;

void main() {
    if((push.hasTexture & (1<<1)) != 0) {
        mat3 TBN = mat3(T, B, N);
        vec3 bump = texture(normalTex, uv).rgb * 2.0 - 1.0;
        bump.xy = bump.xy * -1;
        bump.z = sqrt(1.0 - clamp(dot(bump.xy, bump.xy), 0, 1));
        outNormalOcclusion.xyz = normalize(TBN * bump);
    } else
        outNormalOcclusion.xyz = N, 1.0;
    if((push.hasTexture & (1<<5)) != 0) {
        outEmission = texture(emissionTex, uv).rgba;
    } else
        outEmission = vec4(push.emission.xyz * push.emission.w, 1.0);
    if((push.hasTexture & (1<<0)) != 0)
        outAlbedo.rgb = vertColor * texture(baseTex, uv).rgb;
    else
        outAlbedo.rgb = vertColor;
    if((push.hasTexture & (1<<2)) != 0)
        outMetalRough.x = texture(metalnessTex, uv).x;
    else
        outMetalRough.x = push.metalness;
    if((push.hasTexture & (1<<3)) != 0)
        outMetalRough.y = texture(roughnessTex, uv).x;
    else
        outMetalRough.y = push.roughness;
    if((push.hasTexture & (1<<4)) != 0) {
        outNormalOcclusion.w = texture(occlusionTex, uv).r;
    } else
        outNormalOcclusion.w = 0.0;
    outMtlId = push.mtlId;
    outObjId = modelUbo.objId;
}