#version 450

#define MAX_LIGHT_NUM 64

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 color;

// layout(location = 0) out vec3 outPos;
// layout(location = 1) out vec3 lightPos;


layout(set = 0, binding = 1) uniform ModelUbo {
    mat4 transformMat;
    mat4 normalMat;
    mat4 rotMat;
    int objId;
} modelUbo;

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
    vec4 environment;
} globalUbo;

struct LightInfo {
    vec3 position; // for all
    vec4 emission; // for all
    vec3 direction; // for spot and directional
    mat4 projViewMat;
    int type; // point, spot, directional
    int shadowmap;
    float radius; // for point and spot
    float outerAngle; // for spot
    float innerAngleRatio; // for spot
};

layout(set = 0, binding = 2) uniform LightUbo {
    int lightNum;
    LightInfo infos[MAX_LIGHT_NUM];
} lightUbo;

layout(push_constant) uniform Push {
    mat4 depthPV;
} push;

void main() {
    // outPos = inPos;
    // lightPos = lightUbo.infos[0].position;
    gl_Position = push.depthPV * modelUbo.transformMat * vec4(inPos, 1.0);
}