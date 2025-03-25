#ifndef SKYBOX_H
#define SKYBOX_H

#include "renderer.h"

typedef struct Renderer_T *Renderer;

typedef struct SkyBox_T {
    VkuBuffer buffer;
    VkuUniformBuffer uniBuffer;
    VkuTexture2DArray skyboxTextures;
    VkuTextureSampler skyboxTexSampler;
    VkuDescriptorSet descriptorSet;
    VkuPipeline pipeline;
    VkuRenderStage renderStage;
} SkyBox_T;

typedef SkyBox_T * SkyBox;

typedef struct {
    float playerYpos;
    float iTime;
    vec4 upperColor;
    vec4 lowerColor;
    mat4 model;
} skybox_ubo;

SkyBox createSkyBox(Renderer renderer, VkuRenderStage renderStage);
void destroySkyBox(SkyBox skybox, Renderer renderer);
void renderSkybox(VkuFrame frame, SkyBox skybox, mat4 view, mat4 proj, float playerY, float normTime);

void getLowerSkyColor(float time, vec4 result);
void getUpperSkyColor(float time, vec4 result);
float terrainBrightness(float time);

#endif