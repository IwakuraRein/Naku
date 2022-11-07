#version 450
// #define VULKAN 130


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 color;

layout(location = 0) out vec3 T;
layout(location = 1) out vec3 B;
layout(location = 2) out vec3 N;
layout(location = 3) out vec4 worldPos;
layout(location = 4) out vec3 fragColor;
layout(location = 5) out vec2 fragUv;
// layout(location = 4) out mat4 T2W;

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

void main() {
    worldPos = modelUbo.transformMat * vec4(position, 1.0);
    gl_Position = globalUbo.projectionView * worldPos;

    fragColor = color * push.albedo.xyz;
    T = normalize(mat3(modelUbo.rotMat) * tangent);
    N = normalize(mat3(modelUbo.normalMat) * normal);
    B = cross(N, T);
    fragUv = (uv + push.offsetTilling.xy) * push.offsetTilling.zw;
}