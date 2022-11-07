#version 450

#define MATERIAL_TEXUTRE_NUM 5
#define MAX_OBJECT_NUM 65536

layout(/*input_attachment_index = 0, */set = 1, binding = 0) uniform sampler2D inputColor;
layout(/*input_attachment_index = 1, */set = 1, binding = 1) uniform sampler2D inputNormalOcclusion;
layout(/*input_attachment_index = 2, */set = 1, binding = 2) uniform sampler2D inputAlbedo;
layout(/*input_attachment_index = 3, */set = 1, binding = 3) uniform sampler2D inputMetalRough;
layout(/*input_attachment_index = 4, */set = 1, binding = 4) uniform sampler2D inputEmission;
// layout(/*input_attachment_index = 5, */set = 1, binding = 5) uniform usampler2D inputMtlId;
// layout(/*input_attachment_index = 6, */set = 1, binding = 6) uniform usampler2D inputObjId;
layout(/*input_attachment_index = 6, */set = 1, binding = 5) uniform sampler2D inputDepth;
// layout(set = 1, binding = 7) uniform sampler2D inputShadowmap;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
	vec2 alphaGamma;
	int presentIndex;
} push;

vec3 gammaCorrection(vec3 color, float alpha, float beta, float gamma) {
	vec3 outs = vec3(pow(color.r, gamma), pow(color.g, gamma), pow(color.b, gamma));
	return alpha * outs + beta;
}

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
	vec3 clearColor;
    vec4 environment;
} globalUbo;

vec3 colorHash(uint id) {
	uint a = (id + 0x7ed55d16) + (id << 12);
	a = (a ^ 0xc761c23c) ^ (a >> 19);
	a = (a + 0x165667b1) + (a << 5);
	a = (a + 0xd3a2646c) ^ (a << 9);
	a = (a + 0xfd7046c5) + (a << 3);
	a = (a ^ 0xb55a4f09) ^ (a >> 16);

	uint b = a & 0x3f;
	uint g = (a & 0x7c0) >> 6;
	uint r = (a & 0xf800) >> 11;

	return vec3(r / 32.0, g / 32.0, b / 64.0);
}

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

void main() {
    vec2 gbufferUV = vec2(gl_FragCoord.x/globalUbo.width, gl_FragCoord.y/globalUbo.height);
	if(push.presentIndex == 0) {
		vec3 normal = texture(inputNormalOcclusion, gbufferUV).xyz;
		if(normal.x == 0 && normal.y == 0 && normal.z == 0) {
			outColor = vec4(globalUbo.clearColor, 1.0);
		} else {
			vec3 color = texture(inputColor, gbufferUV).xyz;
			outColor = vec4(gammaCorrection(color, push.alphaGamma.x, 0.0, push.alphaGamma.y), 1.0);
		}
	} else {
		float depth = texture(inputDepth, gbufferUV).x;
		if(depth == 1.0)
			outColor = vec4(0.0, 0.0, 0.0, 1.0);
		else {
			if(push.presentIndex == 1) {
				vec3 normal = texture(inputNormalOcclusion, gbufferUV).xyz;
				outColor = vec4((normal + 1.0) / 2.0, 1.0);
			}
			if(push.presentIndex == 2) {
				vec3 albedo = texture(inputAlbedo, gbufferUV).xyz;
				outColor = vec4(albedo, 1.0);
			}
			if(push.presentIndex == 3) {
				float dist = distZ(depth, globalUbo.clip.z, globalUbo.clip.w) / 10;
				outColor = vec4(dist * push.alphaGamma.x, dist * push.alphaGamma.x, dist * push.alphaGamma.x, 1.0);
			}
			if(push.presentIndex == 4) {
				outColor = vec4(WorldPosFromDepth(depth), 1.0);
			}
			if(push.presentIndex == 5) {
				float metalness = texture(inputMetalRough, gbufferUV).r;
				outColor = vec4(metalness, metalness, metalness, 1.0);
			}
			if(push.presentIndex == 6) {
				float roughness = texture(inputMetalRough, gbufferUV).g;
				outColor = vec4(roughness, roughness, roughness, 1.0);
			}
			if(push.presentIndex == 7) {
				float occlusion = texture(inputNormalOcclusion, gbufferUV).w;
				outColor = vec4(occlusion, occlusion, occlusion, 1.0);
			}
			if(push.presentIndex == 8) {
				vec4 emission = texture(inputEmission, gbufferUV).xyzw;
				emission *= emission.w;
				outColor = vec4(gammaCorrection(emission.xyz, push.alphaGamma.x, 0.0, push.alphaGamma.y), 1.0);
				//outColor = vec4(emission.xyz, 1.0)
			}
			// if(push.presentIndex == 9) {
			// 	uint mtlId = texture(inputMtlId, gbufferUV).x;
			// 	outColor = vec4(colorHash(mtlId), 1.0);
			// }
			// if(push.presentIndex == 10) {
			// 	uint objId = texture(inputObjId, gbufferUV).x;
			// 	outColor = vec4(colorHash(objId), 1.0);
			// }
		}
	}
}