#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DArray texArray;
layout(location = 0) in vec2 uv_in;
layout(location = 1) in float index_in;
layout(location = 2) in float brightness_in;

layout(location = 3) in vec4 fogColor;
layout(location = 4) in float start;
layout(location = 5) in float end;
layout(location = 6) in vec3 fragPos;
layout(location = 7) in vec3 camPos;

void main()
{
    float dist = length(fragPos - camPos);
    float fogFactor = clamp((end - dist) / (end - start), 0.0, 1.0);
    vec4 originalColor = texture(texArray, vec3(uv_in, index_in)) * brightness_in;
    float alpha = fogFactor;
    vec3 finalColor = mix(fogColor.rgb, originalColor.rgb, fogFactor);
    outColor = vec4(finalColor, alpha);
}