#version 450

layout(binding = 1) uniform UniformBufferObject {
    float sunlight;

    vec4 fogColor;
    vec3 camPos;
    float start;
    float end;
    float density;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 modelviewproj;
    mat4 model;
} pushConstants;

layout(location = 0) in float VertPart1_in;
layout(location = 1) in float VertPart2_in;
layout(location = 2) in float VertPart3_in;

layout(location = 0) out vec2 uv_out;
layout(location = 1) out float index_out;
layout(location = 2) out float brightness_out;

layout(location = 3) out vec4 fogColor;
layout(location = 4) out float start;
layout(location = 5) out float end;
layout(location = 6) out vec3 fragPos;
layout(location = 7) out vec3 camPos;

const float expFactor = 0.00444;
const float brightnessScale = 1.0 / 15.0;

void main()
{
    fogColor = ubo.fogColor;
    start = ubo.start;
    end = ubo.end;
    camPos = ubo.camPos;

    uint packedInfo1 = floatBitsToUint(VertPart1_in);
    uint posX = packedInfo1 & 0xFF;
    uint posY = (packedInfo1 >> 8) & 0xFFFFu;
    uint posZ = (packedInfo1 >> 24) & 0xFFu;

    uint packedInfo2 = floatBitsToUint(VertPart2_in);
    uint posUVX = packedInfo2 & 0xFF;
    uint posUVY = (packedInfo2 >> 8) & 0xFF;
    uint ambient = (packedInfo2 >> 16) & 0xFF;
    uint lightValue = packedInfo2 >> 24;

    uint texIndex = floatBitsToUint(VertPart3_in);

    uint blocklight = lightValue & 0xFu;       
    uint sunlight = (lightValue >> 4) & 0xFu;

    float ambientFactor = 1.0 - (float(ambient) * brightnessScale);

    vec4 position = vec4(float(posX), float(posY), float(posZ), 1.0);
    vec3 worldPos = (pushConstants.model * position).xyz;
    gl_Position = pushConstants.modelviewproj * position;
    fragPos = vec3(worldPos.x, 0.0, worldPos.z);

    uv_out = vec2(float(posUVX), float(posUVY));
    
    //float rawBrightness = pow(float(max(float(blocklight), float(sunlight) * ubo.sunlight)), 2) * expFactor;
    float rawBrightness = max(float(blocklight), float(sunlight) * ubo.sunlight) * brightnessScale;
    brightness_out = max(rawBrightness, 0.033) * ambientFactor;
    
    index_out = float(texIndex);
}
