#version 450

#define MATERIAL_TEXUTRE_NUM 5
#define MAX_LIGHT_NUM 64

// #define DEPTH_BIAS_1 0.005
// #define DEPTH_BIAS_2 0.01

// layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColor;
// layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormalOcclusion;
// layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputAlbedo;
// layout(input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputMetalRough;
// layout(input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput inputEmission;
// layout(input_attachment_index = 5, set = 1, binding = 5) uniform usubpassInput inputObjId;
// layout(input_attachment_index = 6, set = 1, binding = 6) uniform subpassInput inputDepth;
layout(/*input_attachment_index = 0, */set = 1, binding = 0) uniform sampler2D inputNormalOcclusion;
layout(/*input_attachment_index = 1, */set = 1, binding = 1) uniform sampler2D inputAlbedo;
layout(/*input_attachment_index = 2, */set = 1, binding = 2) uniform sampler2D inputMetalRough;
layout(/*input_attachment_index = 3, */set = 1, binding = 3) uniform sampler2D inputEmission;
// layout(/*input_attachment_index = 4, */set = 1, binding = 4) uniform usampler2D inputObjId;
layout(/*input_attachment_index = 5, */set = 1, binding = 4) uniform sampler2D inputDepth;
layout(set = 1, binding = 5) uniform sampler2DArray normalShadowmaps;
layout(set = 1, binding = 6) uniform samplerCubeArray omniShadowmaps;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    float time;
    float fov;
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

struct LightInfo {
    vec3 position; // for all
    vec4 emission; // for all
    vec3 direction; // for spot and directional
    mat4 projViewMat;
    int type; // point, spot, directional
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

vec3 WorldPosFromDepth(float z) {
    float x = (gl_FragCoord.x - globalUbo.viewPort.x) / globalUbo.viewPort.z * 2.0 - 1.0;
    float y = (gl_FragCoord.y - globalUbo.viewPort.y) / globalUbo.viewPort.w * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(x, y, z, 1.0);

    vec4 viewSpacePosition = globalUbo.projectionInv * clipSpacePosition;
	// viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = globalUbo.viewInv * viewSpacePosition;
    worldSpacePosition /= worldSpacePosition.w;

    return worldSpacePosition.xyz;
}

float distZ(float z, float zNear, float zFar) {
    float z_n = 2.0 * z - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);

void main() {
    vec2 fragUV = vec2(gl_FragCoord.x / globalUbo.width, gl_FragCoord.y / globalUbo.height);
    float depth = texture(inputDepth, fragUV).x;
    if(depth < 1.0) {
        vec3 normal = texture(inputNormalOcclusion, fragUV).xyz;
        vec3 worldPos = WorldPosFromDepth(depth);
        vec3 diffuseLight = vec3(0.0, 0.0, 0.0);
        vec4 emission = texture(inputEmission, fragUV).xyzw;
        emission *= emission.w;
        vec3 albedo = texture(inputAlbedo, fragUV).xyz;
        int shadowmapCount1 = 0;
        int shadowmapCount2 = 0;
        for(int i = 0; i < lightUbo.lightNum; i++) {
            float contactShadow = 1.0;
            vec3 directionToLight0 = lightUbo.infos[i].position - worldPos;
            vec3 directionToLight = normalize(directionToLight0);
            if(lightUbo.infos[i].shadowmap > 0) {
                float bias = 0.0001;
                if(lightUbo.infos[i].type == 1 || lightUbo.infos[i].type == 2) {
                    vec4 shadowPos = biasMat * lightUbo.infos[i].projViewMat * vec4(worldPos, 1.0);
                    shadowPos /= shadowPos.w;
                    if(shadowPos.z - bias > texture(normalShadowmaps, vec3(shadowPos.xy, shadowmapCount1++)).x)
                    // if(length(directionToLight0) > distZ(texture(normalShadowmaps, vec3(shadowPos.xy, shadowmapCount1++)).x+bias, 0.01, lightUbo.infos[i].radius))
                        continue;
                } else {
                    if(length(directionToLight0) > distZ(texture(omniShadowmaps, vec4(-directionToLight0, shadowmapCount2++)).x + bias, 0.01, lightUbo.infos[i].radius))
                    //if(length(directionToLight0)/* - bias*/ > texture(omniShadowmaps, vec4(-directionToLight0, shadowmapCount2++)).x * 20.0)
                        continue;
                }
            }
            outColor = vec4((normal + 1.0) / 2.0, 1.0);
            if(lightUbo.infos[i].contact_shadow > 0) { // contact shadow
                vec4 lightPos = vec4(lightUbo.infos[i].position, 1.0);
                vec4 lightUV = globalUbo.projectionView * lightPos;
                lightUV /= lightUV.w;
                lightUV.xy = (lightUV.xy + 1.0) / 2.0; // (-1,1) -> (0,1)
                // viewport -> window
                lightUV.x = (lightUV.x * globalUbo.viewPort.z + globalUbo.viewPort.x) / globalUbo.width;
                lightUV.y = (lightUV.y * globalUbo.viewPort.w + globalUbo.viewPort.y) / globalUbo.height;

                vec3 ray = vec3(fragUV, texture(inputDepth, fragUV).x); // (0,1)
                vec3 step = lightUV.xyz - ray;
                float k;
                if(abs(step.x) < 0.01) {
                    if(abs(step.y) < 0.01)
                        continue;
                    else
                        k = 1.0 / globalUbo.height / abs(step.y);
                } else
                    k = 1.0 / globalUbo.width / abs(step.x);
                step *= k;
                const int max_steps = 10;
                // const float thickness = 0.001;
                for(int ii = 0; ii < max_steps; ii++) {
                    ray += step;
                    if(ray.z < 0.0 || ray.z > 1.0)
                        break;
                    const float depth_delta = ray.z - texture(inputDepth, ray.xy).x;
                    if((depth_delta > 0) && (depth_delta < lightUbo.infos[i].contact_shadow)) {
                        //contactShadow = float(ii) / max_steps;
                        contactShadow = 0.0;
                        break;
                    }
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
            diffuseLight += thisLight * contactShadow;
        }
        diffuseLight += (globalUbo.environment.xyz * globalUbo.environment.w);
        outColor = vec4(diffuseLight * albedo + emission.xyz, 1.0);
        // outColor = vec4(contactShadow, contactShadow, contactShadow, 1.0);
    } else
        outColor = vec4(globalUbo.clearColor, 1.0);
}