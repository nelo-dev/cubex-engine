#version 450
layout(location = 0) in vec3 infragPos;
layout(location = 1) in vec3 infragPosStars;

layout(location = 2) in vec4 upperColor;
layout(location = 3) in vec4 lowerColor;
layout(location = 4) in float time;

float uStarDensity = 0.45;
float uStarSize = 0.025;

layout(location = 0) out vec4 outColor;

// Optimized hash function with fewer operations
vec2 hash2(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(vec2(h, h + 123.45)) * 43758.5453);
}

void main() {
    vec3 norm = normalize(infragPos);
    
    // Sky gradient constants
    const float center = 0.15, halfBand = 0.08;
    const float lowerEdge = center - halfBand, upperEdge = center + halfBand;
    float mixFactor = smoothstep(lowerEdge, upperEdge, norm.y);
    vec3 finalColor = mix(lowerColor.rgb, upperColor.rgb, mixFactor);

    // Cube face selection without branching
    vec3 pos = normalize(infragPosStars);
    vec3 absPos = abs(pos);
    float maxAxis = max(absPos.x, max(absPos.y, absPos.z));

    float faceIndex = (absPos.x >= maxAxis) ? ((pos.x > 0.0) ? 0.0 : 1.0) :
                     (absPos.y >= maxAxis) ? ((pos.y > 0.0) ? 2.0 : 3.0) :
                     ((pos.z > 0.0) ? 4.0 : 5.0);

    vec2 uv = (absPos.x >= maxAxis) ? (pos.zy + 1.0) * 0.5 :
              (absPos.y >= maxAxis) ? (pos.xz + 1.0) * 0.5 :
              (pos.xy + 1.0) * 0.5;

    // Optimized star generation
    const float gridSize = 25.0;
    vec2 cell = floor(uv * gridSize);
    vec2 seed = cell + vec2(faceIndex * 100.0);
    vec2 starRand = hash2(seed);
    
    if (starRand.y > 1.0 - uStarDensity) {
        float starSize = uStarSize * (0.7 + 0.3 * sin(time * 4.0 + starRand.x * 100.0));
        vec2 starPos = starRand * 0.8 + 0.1;
        vec2 delta = fract(uv * gridSize) - starPos;
        
        // Changed to hard step for sharp squares
        float d = max(abs(delta.x), abs(delta.y));
        float square = 1.0 - step(starSize, d); // Hard cutoff
        square *= 0.8 + 0.2 * sin(0* 6.0 + starRand.x * 100.0); // Fixed time multiplier
        
        finalColor = mix(finalColor, vec3(1.0), square);
    }

    outColor = vec4(finalColor, 1.0);
}