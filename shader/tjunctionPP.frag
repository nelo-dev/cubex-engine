#version 450 core

layout(binding = 0) uniform sampler2D colorImage;
layout(binding = 1) uniform sampler2D depthImage;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 pixelLength;
layout(location = 0) out vec4 outColor;

// Define a small epsilon for floating-point comparisons
const float EPSILON = 0.0001;

void main(void) {
    // Pre-calculate the flipped coordinate for y-axis inversion
    vec2 flippedCoord = vec2(texCoord.x, 1.0 - texCoord.y);

    // Sample the current pixel's depth
    float depth = texture(depthImage, flippedCoord).r;

    // Early exit if the current pixel's depth is valid (not approximately 1.0)
    if (abs(depth - 1.0) > EPSILON) {
        outColor = texture(colorImage, flippedCoord);
        return;
    }

    // Define the surrounding pixel offsets using the input 'pixelLength'
    vec2 offsets[8] = vec2[](
        vec2(-pixelLength.x,  pixelLength.y),   // Top-left
        vec2( 0.0,               pixelLength.y), // Top-center
        vec2( pixelLength.x,    pixelLength.y), // Top-right
        vec2(-pixelLength.x,  0.0),             // Mid-left
        vec2( pixelLength.x,  0.0),             // Mid-right
        vec2(-pixelLength.x, -pixelLength.y),   // Bottom-left
        vec2( 0.0,              -pixelLength.y), // Bottom-center
        vec2( pixelLength.x,  -pixelLength.y)   // Bottom-right
    );

    // Initialize the color sum and count of valid pixels
    vec4 colorSum = vec4(0.0);
    int validCount = 0;
    int invalidDepthCount = 0;

    // Loop over surrounding pixels and accumulate valid colors
    for (int i = 0; i < 8; ++i) {
        vec2 sampleCoord = flippedCoord + offsets[i];

        // Optional: Clamp the sample coordinates to [0.0, 1.0] to avoid sampling outside the texture
        sampleCoord = clamp(sampleCoord, vec2(0.0), vec2(1.0));

        float surroundingDepth = texture(depthImage, sampleCoord).r;

        // If surrounding depth is approximately 1.0, increment the invalid depth count
        if (abs(surroundingDepth - 1.0) < EPSILON) {
            invalidDepthCount++;
        } else {
            // Sum the colors of valid surrounding pixels
            colorSum += texture(colorImage, sampleCoord);
            validCount++;
        }
    }

    // Only interpolate if 3 or fewer surrounding pixels have a depth of 1.0
    if (invalidDepthCount > 3) {
        outColor = texture(colorImage, flippedCoord);
    } else if (validCount > 0) {
        // Average the colors from the valid surrounding pixels
        outColor = colorSum / float(validCount);
    } else {
        outColor = texture(colorImage, flippedCoord);
    }
}