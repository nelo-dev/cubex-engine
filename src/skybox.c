#include "skybox.h"
#include "world.h"

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 0.0f,
     1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f,

    -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 4.0f,
    -1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 4.0f,
    -1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 4.0f,
    -1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 4.0f,
    -1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 4.0f,
    -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 4.0f,

     1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 5.0f,
     1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 5.0f,
     1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 5.0f,
     1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 5.0f,
     1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 5.0f,
     1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 5.0f,

    -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 1.0f,
     1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 1.0f,
    -1.0f, -1.0f,  1.0f,   0.0f, 0.0f, 1.0f,

    -1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 2.0f,
    -1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 2.0f,
     1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 2.0f,
     1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 2.0f,
     1.0f,  1.0f, -1.0f,   1.0f, 0.0f, 2.0f,
    -1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 2.0f,

    -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 3.0f,
     1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 3.0f,
     1.0f, -1.0f,  1.0f,   1.0f, 1.0f, 3.0f,
     1.0f, -1.0f,  1.0f,   1.0f, 1.0f, 3.0f,
    -1.0f, -1.0f,  1.0f,   0.0f, 1.0f, 3.0f,
    -1.0f, -1.0f, -1.0f,   0.0f, 0.0f, 3.0f
};

SkyBox createSkyBox(Renderer renderer, VkuRenderStage renderStage)
{
    SkyBox skybox = calloc(1, sizeof(SkyBox_T));

    size_t size = sizeof(skyboxVertices);
    VkuBuffer stagingBuffer = vkuCreateVertexBuffer(renderer->context->memoryManager, size, VKU_BUFFER_USAGE_CPU_TO_GPU);
    vkuSetVertexBufferData(renderer->context->memoryManager, stagingBuffer, skyboxVertices, size);
    skybox->buffer = vkuCreateVertexBuffer(renderer->context->memoryManager, size, VKU_BUFFER_USAGE_GPU_ONLY);
    vkuCopyBuffer(renderer->context->memoryManager, &stagingBuffer, &(skybox->buffer), &size, 1);
    vkuDestroyVertexBuffer(stagingBuffer, renderer->context->memoryManager, VK_TRUE);

    skybox->uniBuffer = vkuCreateUniformBuffer(renderer->context, sizeof(skybox_ubo), vkuPresenterGetFramesInFlight(renderer->presenter));

    int channels, height, width;
    uint8_t *images[] = {
        vkuLoadImage("./res/tex/skybox/back.png", &width, &height, &channels),
        vkuLoadImage("./res/tex/skybox/front.png", &width, &height, &channels),
        vkuLoadImage("./res/tex/skybox/top.png", &width, &height, &channels),
        vkuLoadImage("./res/tex/skybox/bottom.png", &width, &height, &channels),
        vkuLoadImage("./res/tex/skybox/left.png", &width, &height, &channels),
        vkuLoadImage("./res/tex/skybox/right.png", &width, &height, &channels)};

    VkuTexture2DArrayCreateInfo texArrayInfo = {
        .channels = channels,
        .height = height,
        .layerCount = sizeof(images) / sizeof(uint8_t*),
        .mipLevels = 1,
        .pixelDataArray = images,
        .width = width
    };

    skybox->skyboxTextures = vkuCreateTexture2DArray(renderer->context, &texArrayInfo);

    for (uint32_t i = 0; i < sizeof(images) / sizeof(uint8_t *); i++)
        free(images[i]);

    VkuTextureSamplerCreateInfo skyboxSamplerInfo = {
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapLevels = 1,
        .repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    };

    skybox->skyboxTexSampler = vkuCreateTextureSampler(renderer->context, &skyboxSamplerInfo);

    VkuDescriptorSetAttribute setAttrib1 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_UNIFORM_BUFFER, .uniformBuffer = skybox->uniBuffer};
    VkuDescriptorSetAttribute setAttrib2 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_SAMPLER, .tex2DArray = skybox->skyboxTextures, .sampler = skybox->skyboxTexSampler};
    VkuDescriptorSetAttribute setAttribs[] = {setAttrib1, setAttrib2};

    VkuDescriptorSetCreateInfo setInfo = {
        .attributeCount = sizeof(setAttribs) / sizeof(VkuDescriptorSetAttribute),
        .attributes = setAttribs,
        .descriptorCount = vkuPresenterGetFramesInFlight(renderer->presenter),
        .renderStage = renderStage
    };

    skybox->descriptorSet = vkuCreateDescriptorSet(&setInfo);

    VkuVertexAttribute vertexAttrib1 = {.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0};
    VkuVertexAttribute vertexAttrib2 = {.format = VK_FORMAT_R32G32_SFLOAT, .offset = sizeof(float) * 3};
    VkuVertexAttribute vertexAttrib3 = {.format = VK_FORMAT_R32_SFLOAT, .offset = sizeof(float) * 5};
    VkuVertexAttribute vertexAttributes[] = {vertexAttrib1, vertexAttrib2, vertexAttrib3};
    VkuVertexLayout vertexLayout = {.attributeCount = 3, .attributes = vertexAttributes, .vertexSize = 6 * sizeof(float)};

    VkuPipelineCreateInfo pipelineInfo = {
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .depthCompareMode = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthTestEnable = VK_TRUE,
        .depthTestWrite = VK_FALSE,
        .descriptorSet = skybox->descriptorSet,
        .fragmentShaderSpirV = vkuReadFile("./shader/skybox_frag.spv", &pipelineInfo.fragmentShaderLength),
        .polygonMode = VK_POLYGON_MODE_FILL,
        .renderStage = renderStage,
        .vertexLayout = vertexLayout,
        .vertexShaderSpirV = vkuReadFile("./shader/skybox_vert.spv", &pipelineInfo.vertexShaderLength),
    };

    skybox->pipeline = vkuCreatePipeline(renderer->context, &pipelineInfo);
    free(pipelineInfo.fragmentShaderSpirV);
    free(pipelineInfo.vertexShaderSpirV);

    return skybox;
}

void renderSkybox(VkuFrame frame, SkyBox skybox, mat4 view, mat4 proj, float playerY, float normTime)
{
    mat4 skybox_view_mat = GLM_MAT4_IDENTITY_INIT;
    mat3 view_mat3 = GLM_MAT3_IDENTITY_INIT;
    glm_mat4_pick3(view, view_mat3);
    glm_mat4_ins3(view_mat3, skybox_view_mat);

    mat4 viewproj = GLM_MAT4_IDENTITY_INIT;
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_mul(proj, skybox_view_mat, viewproj);
    glm_rotate(model, normTime * M_PI * 2, (vec3) {0.0f, 1.0f, 1.0f});

    vkuFrameBindPipeline(frame, skybox->pipeline);

    #define MAX(a, b) ((a) > (b) ? (a) : (b))

    skybox_ubo ubo = {
        .iTime = glfwGetTime(),
        .playerYpos = playerY,
    };

    getLowerSkyColor(normTime, ubo.lowerColor);
    getUpperSkyColor(normTime, ubo.upperColor);
    glm_mat4_copy(model, ubo.model);

    vkuFrameUpdateUniformBuffer(frame, skybox->uniBuffer, &ubo);
    vkuFramePipelinePushConstant(frame, skybox->pipeline, &viewproj, sizeof(mat4)); 
    vkuFrameDrawVertexBuffer(frame, skybox->buffer, 36);
}

void destroySkyBox(SkyBox skybox, Renderer renderer)
{
    vkuDestroyPipeline(renderer->context, skybox->pipeline);
    vkuDestroyDescriptorSet(skybox->descriptorSet);
    vkuDestroyUniformBuffer(renderer->context, skybox->uniBuffer);
    vkuDestroyVertexBuffer(skybox->buffer, renderer->context->memoryManager, VK_TRUE);
    vkuDestroyTexture2DArray(renderer->context, skybox->skyboxTextures);
    vkuDestroyTextureSampler(renderer->context, skybox->skyboxTexSampler);

    free(skybox);
}

#define PI  3.14159265358979323846
#define PERIODS 5

vec4 skyColor[PERIODS][2] = {
    {{0.804, 0.671, 0.522, 1.0}, {0.482, 0.604, 0.894, 1.0}}, //sunrise
    {{0.714, 0.816, 1.00, 1.00}, {0.302, 0.459, 1.0, 1.0}}, //noon
    {{0.920, 0.355, 0.255, 1.0}, {0.282, 0.322, 0.459, 1.0}}, // evening
    {{0.002, 0.002, 0.006, 1.0}, {0.0015, 0.001, 0.004, 1.0}}, //night
    {{0.804, 0.671, 0.522, 1.0}, {0.482, 0.604, 0.894, 1.0}}, // sunrise
};

float periods[] = {0.0, 0.1, 0.4, 0.5, 0.6, 0.9, 1.0};
float brightness[] = {0.5, 1.0, 0.75, 0.01, 0.5};

void interpolateColor(const vec4 colorA, const vec4 colorB, float factor, vec4 result) {
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    float easedFactor = sin(factor * (PI / 2));

    for (int i = 0; i < 4; ++i) {
        result[i] = colorA[i] + easedFactor * (colorB[i] - colorA[i]);
    }
}

void getLowerSkyColor(float time, vec4 result) {
    if (time >= periods[0] && time <= periods[1]) {
        interpolateColor(skyColor[0][0], skyColor[1][0], time * 10.0f, result);
        return;
    } else if (time > periods[1] && time <= periods[2]) {
        glm_vec4_copy(skyColor[1][0], result);
    } else if (time > periods[2] && time <= periods[3]) {
        interpolateColor(skyColor[1][0], skyColor[2][0], (time - 0.4) * 10.0f, result);
        return;
    } else if (time > periods[3] && time <= periods[4]) {
        interpolateColor(skyColor[2][0], skyColor[3][0], (time - 0.5) * 10.0f, result);
        return;
    } else if (time > periods[4] && time <= periods[5]) {
        glm_vec4_copy(skyColor[3][0], result);
    } else if (time > periods[5] && time <= periods[6]) {
        interpolateColor(skyColor[3][0], skyColor[4][0], (time - 0.9) * 10.0f, result);
        return;
    }
}

void getUpperSkyColor(float time, vec4 result) {
    if (time >= periods[0] && time <= periods[1]) {
        interpolateColor(skyColor[0][1], skyColor[1][1], time * 10.0f, result);
        return;
    } else if (time > periods[1] && time <= periods[2]) {
        glm_vec4_copy(skyColor[1][1], result);
    } else if (time > periods[2] && time <= periods[3]) {
        interpolateColor(skyColor[1][1], skyColor[2][1], (time - 0.4) * 10.0f, result);
        return;
    } else if (time > periods[3] && time <= periods[4]) {
        interpolateColor(skyColor[2][1], skyColor[3][1], (time - 0.5) * 10.0f, result);
        return;
    } else if (time > periods[4] && time <= periods[5]) {
        glm_vec4_copy(skyColor[3][1], result);
    } else if (time > periods[5] && time <= periods[6]) {
        interpolateColor(skyColor[3][1], skyColor[4][1], (time - 0.9) * 10.0f, result);
        return;
    }
}

float terrainBrightness(float time)
{
    if (time >= periods[0] && time <= periods[1]) {
        return brightness[0] + time * 10.0f * (brightness[1] - brightness[0]);
    } else if (time > periods[1] && time <= periods[2]) {
        return brightness[1];
    } else if (time > periods[2] && time <= periods[3]) {
        return brightness[1] + (time - 0.4) * 10.0f * (brightness[2] - brightness[1]);
    } else if (time > periods[3] && time <= periods[4]) {
        return brightness[2] + (time - 0.5) * 10.0f * (brightness[3] - brightness[2]);
    } else if (time > periods[4] && time <= periods[5]) {
        return brightness[3];
    } else if (time > periods[5] && time <= periods[6]) {
        return brightness[3] + (time - 0.9) * 10.0f * (brightness[4] - brightness[3]);
    }
}