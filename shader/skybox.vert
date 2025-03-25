#version 450
layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 viewproj;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    float playerYpos;
    float iTime;
    vec4 upperColor;
    vec4 lowerColor;
    mat4 model;
} ubo;

layout(location = 0) out vec3 outfragPos;
layout(location = 1) out vec3 outfragPosStars;

layout(location = 2) out vec4 upperColorOut;
layout(location = 3) out vec4 lowerColorOut;
layout(location = 4) out float timeOut; 

void main() {
    outfragPos = inPosition;
    outfragPosStars = (ubo.model * vec4(inPosition, 1.0)).xyz;
    timeOut = ubo.iTime;

    upperColorOut = ubo.upperColor;
    lowerColorOut = ubo.lowerColor;

    vec4 pos = pushConstants.viewproj * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
}