#include "renderer.h"
#include "game.h"

#include <stdatomic.h>
#include <string.h>

void rendererHandleInput(Renderer renderer)
{
    updateInputHandler(renderer->inputHandler);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_F11) == KEY_RELEASED) {
        vkuWindowToggleFullscreen(renderer->presenter->window);
        renderer->inputHandler->fullscreen_toggled = true;
    }

    if (getKeyState(renderer->inputHandler, GLFW_KEY_G) == KEY_RELEASED) {
        long used_memory = get_used_memory_kb();
        if (used_memory >= 0) {
            printf("Used RAM: %ld KB\n", used_memory);
        } else {
            printf("Failed to retrieve memory usage.\n");
        }
    }

    if (getKeyState(renderer->inputHandler, GLFW_KEY_ESCAPE) == KEY_PRESSED)
        if (glfwGetInputMode(renderer->presenter->window->glfwWindow, GLFW_CURSOR) != GLFW_CURSOR_NORMAL)
            glfwSetInputMode(renderer->presenter->window->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetWindowShouldClose(renderer->presenter->window->glfwWindow, true);
    else if (glfwGetMouseButton(renderer->presenter->window->glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        glfwSetInputMode(renderer->presenter->window->glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_F12) == KEY_RELEASED) {
        printf("VRAM: %.2f MB in use!\n", (double) vkuMemoryMamgerGetAllocatedMemorySize(renderer->context->memoryManager) / 1000000.0f);
    }

    if (glfwGetInputMode(renderer->presenter->window->glfwWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        swingCamera(renderer->camera, renderer->inputHandler->mouseRelX, renderer->inputHandler->mouseRelY, renderer->inputHandler->mouseSensitivity);
    }

    if (getKeyState(renderer->inputHandler, GLFW_KEY_W) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_FRONT);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_S) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_BACK);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_A) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_LEFT);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_D) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_RIGHT);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_SPACE) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_UP);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_LEFT_SHIFT) == KEY_HELD)
        moveCamera(renderer->camera, CAM_DIRECTION_DOWN);

    if (getKeyState(renderer->inputHandler, GLFW_KEY_LEFT_CONTROL) == KEY_HELD)
        renderer->camera->zoom = 60.0f;
    else 
        renderer->camera->zoom = 0.0f;

    Game game = (Game) renderer->game;

    if (getKeyState(renderer->inputHandler, GLFW_KEY_R) == KEY_HELD) {
        int ix, iz, iy;
        CameraToWorldCoords(renderer->camera->camPos.x, renderer->camera->camPos.y, renderer->camera->camPos.z, &ix, &iy, &iz);
        setBlockInWorld(game->world, ix, iy, iz, 1);
    }

    if (getKeyState(renderer->inputHandler, GLFW_KEY_P) == KEY_RELEASED)
    {
        game->renderer->camera->camPos.x = rand() % 1000000;
        game->renderer->camera->camPos.z = rand() % 1000000;
    }

    if (getKeyState(renderer->inputHandler, GLFW_KEY_F7) == KEY_RELEASED)
    {
        if (renderer->debugViewMode < 3) 
            renderer->debugViewMode++;
        else
            renderer->debugViewMode = 0;
    }

    static int msaa = true;
    if (getKeyState(renderer->inputHandler, GLFW_KEY_M) == KEY_RELEASED)
    {
        msaa = !msaa;
        vkuRenderStageSetMSAA(renderer->blockRenderStage, (msaa) ? vkuContextGetMaxSampleCount(renderer->context) : VK_SAMPLE_COUNT_1_BIT);
    }
}

VkuTexture2DArray createBlockTextureArray(Registry registry, Renderer renderer)
{
    uint8_t **data = (uint8_t **)malloc(registry->blockList->infoArray.size * 6 * sizeof(uint8_t *));
    int width, height, channels;
    BlockInfo * infos = (BlockInfo* ) registry->blockList->infoArray.data;

    for (uint32_t i = 0; i < registry->blockList->infoArray.size; i++)
    {
        for (uint32_t j = 0; j < 6; j++)
        {
            data[i * 6 + j] = vkuLoadImage(infos[i].texPaths[j], &width, &height, &channels);

            if (height != 16 || width != 16)
                fprintf(stderr, "Wrong tex dim!");

            infos[i].texArrayIndices[j] = i * 6 + j;
        }
    }

    VkuTexture2DArrayCreateInfo texInfo = {
        .pixelDataArray = data,
        .height = height,
        .layerCount = registry->blockList->infoArray.size * 6,
        .mipLevels = MIP_LEVELS,
        .width = width
    };

    VkuTexture2DArray arrayTex = vkuCreateTexture2DArray(renderer->context, &texInfo);

    for (uint32_t i = 0; i < registry->blockList->infoArray.size * 6; i++)
    {
        free(data[i]);
    }

    free(data);

    return arrayTex;
}

void renderWorld(VkuFrame frame, Renderer renderer, mat4 view, mat4 proj)
{
    pthread_mutex_lock(&renderer->swapRendCksMutex);

    Game game = (Game) renderer->game;
    vec3 camPos, camFront;
    dvec3_to_vec3(renderer->camera->camPos, camPos);
    dvec3_to_vec3(renderer->camera->camFront, camFront);
    World world = game->world;

    if (world == NULL || !atomic_load(&world->created))
    {
        pthread_mutex_unlock(&renderer->swapRendCksMutex);
        return;
    }

    #define MAX(a, b) ((a) > (b) ? (a) : (b))

    vkuFrameBindPipeline(frame, renderer->blockPipeline);

    float normalizedTime = getNormalizedWorldTime(world);
    float sunlight = terrainBrightness(normalizedTime);

    block_ubo buffer = {
        .sunlight = sunlight,
        .camPos = {fmod(camPos[0], (float) SC_LEN), 0.0, fmod(camPos[2], (float) SC_LEN)},
        .start = (world->renderDistance / 2.0f - 2.25) * SC_LEN,
        .end = (world->renderDistance / 2.0f - 2.0) * SC_LEN,
        .density = 1.0f
    };

    getLowerSkyColor(normalizedTime, buffer.fogColor);

    vkuFrameUpdateUniformBuffer(frame, renderer->sceneUniBuffer, &buffer);

    mat4 viewproj = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_mul(proj, view, viewproj);

    Plane frustum[6];
    ExtractFrustumPlanes(frustum, viewproj);

    float fovRadians = glm_rad(90.0f);
    float halfFovCos = cosf(fovRadians);

    for (int locZ = 1; locZ < world->renderDistance - 1; locZ++) {
        for (int locX = 1; locX < world->renderDistance - 1; locX++) {
            Chunk chunk = world->renderedChunks[locX + locZ * world->renderDistance];

            vec2 distVec;
            glm_vec2_sub((vec2) {(float) locX, (float) locZ}, (vec2) {(float) world->renderDistance / 2.0f, (float) world->renderDistance / 2.0f}, distVec);
            if (glm_vec2_norm(distVec) > (float) world->renderDistance / 2.0f - 1)
                continue;

            if (chunk == NULL || !atomic_load(&chunk->created)) {
                continue;
            }

            chunkMesh mesh = chunk->chunkMesh;

            if (isChunkMeshReadyForSwap(mesh)) {
                chunkMeshSwap(game->renderer, mesh);
            }

            float xoffset = (float) ((int) renderer->camera->camPos.x / SC_LEN) * SC_LEN;
            float zoffset = (float) ((int) renderer->camera->camPos.z / SC_LEN) * SC_LEN;

            vec3 aabbMin = { (float)chunk->chunkX * SC_LEN - xoffset, 0.0f, (float)chunk->chunkZ * SC_LEN - zoffset };
            vec3 aabbMax = { (float)chunk->chunkX * SC_LEN - xoffset + SC_LEN, SC_CNT * SC_LEN, (float)chunk->chunkZ * SC_LEN - zoffset + SC_LEN };

            if (!AABBInFrustum(frustum, aabbMin, aabbMax)) {
                continue;
            }

            mat4 modelviewproj = GLM_MAT4_IDENTITY_INIT;
            mat4 model = GLM_MAT4_IDENTITY_INIT;
            glm_translate(modelviewproj, (vec3) {(float) chunk->chunkX * SC_LEN - xoffset, 0.0f, (float) chunk->chunkZ * SC_LEN - zoffset});
            glm_mat4_copy(modelviewproj, model);
            glm_mat4_mul(viewproj, modelviewproj, modelviewproj);

            struct pushConstant {
                mat4 modelviewproj;
                mat4 model;
            };

            struct pushConstant pushC;
            glm_mat4_copy(model, pushC.model);
            glm_mat4_copy(modelviewproj, pushC.modelviewproj);
            
            vkuFramePipelinePushConstant(frame, renderer->blockPipeline, &pushC, sizeof(struct pushConstant));

            if (mesh->opaqueBuffer[MESH_SECOND] != NULL && mesh->bufferSize[MESH_SECOND] > 0) {
                vkuFrameDrawVertexBuffer(frame, mesh->opaqueBuffer[MESH_SECOND], mesh->bufferSize[MESH_SECOND] / (sizeof(float) * VERTEX_FLOAT_COUNT));
            }
        }
    }

    if (renderer->debugViewMode > 0) {
        vkuFrameBindPipeline(frame, renderer->debugPipeline);

        for (int locZ = 1; locZ < world->renderDistance - 1; locZ++) {
            for (int locX = 1; locX < world->renderDistance - 1; locX++) {
                Chunk chunk = world->renderedChunks[locX + locZ * world->renderDistance];

                vec2 distVec;
                glm_vec2_sub((vec2) {(float) locX, (float) locZ}, (vec2) {(float) world->renderDistance / 2.0f, (float) world->renderDistance / 2.0f}, distVec);
                if (glm_vec2_norm(distVec) > (float) 6)
                    continue;

                if (chunk == NULL || !atomic_load(&chunk->created)) {
                    continue;
                }

                chunkMesh mesh = chunk->chunkMesh;

                float xoffset = (float) ((int) renderer->camera->camPos.x / SC_LEN) * SC_LEN;
                float zoffset = (float) ((int) renderer->camera->camPos.z / SC_LEN) * SC_LEN;

                vec3 aabbMin = { (float)chunk->chunkX * SC_LEN - xoffset, 0.0f, (float)chunk->chunkZ * SC_LEN - zoffset };
                vec3 aabbMax = { (float)chunk->chunkX * SC_LEN - xoffset + SC_LEN, SC_CNT * SC_LEN, (float)chunk->chunkZ * SC_LEN - zoffset + SC_LEN };

                if (!AABBInFrustum(frustum, aabbMin, aabbMax)) {
                    continue;
                }

                for (uint32_t i = 0; i < SC_CNT; i++) {
                    mat4 modelviewproj = GLM_MAT4_IDENTITY_INIT;
                    mat4 model = GLM_MAT4_IDENTITY_INIT;
                    glm_translate(modelviewproj, (vec3) {(float) chunk->chunkX * SC_LEN - xoffset, i * SC_LEN, (float) chunk->chunkZ * SC_LEN - zoffset});
                    glm_mat4_copy(modelviewproj, model);
                    glm_mat4_mul(viewproj, modelviewproj, modelviewproj);

                    struct pushConstant {
                        mat4 modelviewproj;
                    };
        
                    struct pushConstant pushC;
                    glm_mat4_copy(modelviewproj, pushC.modelviewproj);

                    if (i == 0 && renderer->debugViewMode >= 2) {
                        vkuFramePipelinePushConstant(frame, renderer->debugPipeline, &pushC, sizeof(struct pushConstant));
                        vkuFrameDrawVertexBuffer(frame, renderer->chunkDebugMesh, 36);
                    }

                    if (chunk->subchunks[i] != NULL && (renderer->debugViewMode == 1 || renderer->debugViewMode == 3)) {
                        vkuFramePipelinePushConstant(frame, renderer->debugPipeline, &pushC, sizeof(struct pushConstant));
                        vkuFrameDrawVertexBuffer(frame, renderer->subchunkDebugMesh, 36);
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&renderer->swapRendCksMutex);
}

void generateCubeTriangles(float x, float y, float z, float r, float g, float b, float padding, float* vertices) {
    // Ensure padding doesn't exceed half of any dimension
    float px = (x > 2 * padding) ? padding : x / 2;
    float py = (y > 2 * padding) ? padding : y / 2;
    float pz = (z > 2 * padding) ? padding : z / 2;

    // Define 8 unique vertices with padding applied
    float v[] = {
        px,  py,  pz,        // Bottom-left-front
        x - px,  py,  pz,    // Bottom-right-front
        x - px,  y - py,  pz,// Top-right-front
        px,  y - py,  pz,    // Top-left-front
        px,  py,  z - pz,    // Bottom-left-back
        x - px,  py,  z - pz,// Bottom-right-back
        x - px,  y - py,  z - pz, // Top-right-back
        px,  y - py,  z - pz // Top-left-back
    };

    // Indices for triangle strips (no internal diagonal)
    int indices[] = {
        0, 1, 2,  2, 3, 0,  // Front face
        1, 5, 6,  6, 2, 1,  // Right face
        5, 4, 7,  7, 6, 5,  // Back face
        4, 0, 3,  3, 7, 4,  // Left face
        3, 2, 6,  6, 7, 3,  // Top face
        4, 5, 1,  1, 0, 4   // Bottom face
    };

    // Fill vertex array with position and color
    for (int i = 0; i < 36; i++) {
        int idx = indices[i] * 3;  // Get position index
        vertices[i * 6] = v[idx];      // x
        vertices[i * 6 + 1] = v[idx+1];// y
        vertices[i * 6 + 2] = v[idx+2];// z
        vertices[i * 6 + 3] = r;       // r
        vertices[i * 6 + 4] = g;       // g
        vertices[i * 6 + 5] = b;       // b
    }
}

double getFrameDeltaTime()
{
    static struct timespec lastTime;
    static int firstCall = 1;
    struct timespec currentTime;
    clock_gettime(1, &currentTime);

    if (firstCall)
    {
        lastTime = currentTime;
        firstCall = 0;
    }

    double deltaTime = (currentTime.tv_sec - lastTime.tv_sec) + (currentTime.tv_nsec - lastTime.tv_nsec) / 1000000000.0;

    lastTime = currentTime;
    return deltaTime;
}

void * renderFunction(void * arg) 
{
    Renderer renderer = (Renderer) arg;

    VkuContextCreateInfo contextCreateInfo = {
        .enableValidation = VK_FALSE,
        .applicationName = "Cubex Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .usage = VKU_CONTEXT_USAGE_PRESENTATION};

    renderer->context = vkuCreateContext(&contextCreateInfo);

    VkuPresenterCreateInfo presenterCreateInfo = {
        .context = renderer->context,
        .width = 1152,
        .height = 720,
        .windowTitle = "Cubex Engine",
        .windowIconPath = NULL,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .framesInFlight = 2,
    };

    renderer->presenter = vkuCreatePresenter(&presenterCreateInfo);

    renderer->inputHandler = createInputHandler(renderer->presenter->window);
    renderer->camera = createCamera((dvec3) {.x = -1.0f, .y = 128.0f, .z = -1.0f});

    VkuRenderStageCreateInfo renderStageCreateInfo = {
        .msaaSamples = vkuContextGetMaxSampleCount(renderer->context),
        .presenter = renderer->presenter,
        .options = VKU_RENDER_OPTION_COLOR_IMAGE | VKU_RENDER_OPTION_DEPTH_IMAGE,
        .enableDepthTesting = VK_TRUE};

    renderer->blockRenderStage = vkuCreateRenderStage(&renderStageCreateInfo);

    renderer->blockArray = createBlockTextureArray(renderer->registry, renderer);

    VkuTextureSamplerCreateInfo samplerInfo = {
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapLevels = MIP_LEVELS,
        .repeatMode = VK_SAMPLER_ADDRESS_MODE_REPEAT};

    renderer->texSampler = vkuCreateTextureSampler(renderer->context, &samplerInfo);

    renderer->sceneUniBuffer = vkuCreateUniformBuffer(renderer->context, sizeof(block_ubo), vkuPresenterGetFramesInFlight(renderer->presenter));

    VkuDescriptorSetAttribute setAttrib1 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_SAMPLER, .tex2DArray = renderer->blockArray, .sampler = renderer->texSampler};
    VkuDescriptorSetAttribute setAttrib2 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_UNIFORM_BUFFER, .uniformBuffer = renderer->sceneUniBuffer};
    VkuDescriptorSetAttribute setAttribs[] = {setAttrib1, setAttrib2};
    VkuDescriptorSetCreateInfo setInfo = {.attributeCount = 2, .attributes = setAttribs, .descriptorCount = vkuPresenterGetFramesInFlight(renderer->presenter), .renderStage = renderer->blockRenderStage};
    renderer->descriptorSet = vkuCreateDescriptorSet(&setInfo);

    VkuVertexAttribute attrib1 = {.format = VK_FORMAT_R32_SFLOAT, .offset = 0};
    VkuVertexAttribute attrib2 = {.format = VK_FORMAT_R32_SFLOAT, .offset = sizeof(float)};
    VkuVertexAttribute attrib3 = {.format = VK_FORMAT_R32_SFLOAT, .offset = 2*sizeof(float)};
    VkuVertexAttribute vertexAttribs[] = {attrib1, attrib2, attrib3};
    VkuVertexLayout vertexLayout = {.attributeCount = 3, .attributes = vertexAttribs, .vertexSize = 3 * sizeof(float)};
    VkuPipelineCreateInfo pipelineInfo = {
        .fragmentShaderSpirV = vkuReadFile("./shader/block_frag.spv", &pipelineInfo.fragmentShaderLength),
        .vertexShaderSpirV = vkuReadFile("./shader/block_vert.spv", &pipelineInfo.vertexShaderLength),
        .polygonMode = VK_POLYGON_MODE_FILL,
        .descriptorSet = renderer->descriptorSet,
        .vertexLayout = vertexLayout,
        .depthTestEnable = VK_TRUE,
        .depthTestWrite = VK_TRUE,
        .depthCompareMode = VK_COMPARE_OP_LESS,
        .renderStage = renderer->blockRenderStage,
        .cullMode = VK_CULL_MODE_BACK_BIT};

    renderer->blockPipeline = vkuCreatePipeline(renderer->context, &pipelineInfo);

    free(pipelineInfo.fragmentShaderSpirV);
    free(pipelineInfo.vertexShaderSpirV);

    VkuTexture2D colorOutput = vkuRenderStageGetColorOutput(renderer->blockRenderStage);
    VkuTexture2D depthOutput = vkuRenderStageGetDepthOutput(renderer->blockRenderStage);

    VkuRenderStageCreateInfo ppRenderStageCreateInfo = {
        .msaaSamples = VK_SAMPLE_COUNT_1_BIT,
        .presenter = renderer->presenter,
        .options = VKU_RENDER_OPTION_PRESENTER,
        .enableDepthTesting = VK_FALSE};

    VkuRenderStage ppRenderStage = vkuCreateRenderStage(&ppRenderStageCreateInfo);

    VkuDescriptorSetAttribute ppSetAttrib1 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_SAMPLER, .sampler = renderer->texSampler, .tex2D = colorOutput};
    VkuDescriptorSetAttribute ppSetAttrib2 = {.type = VKU_DESCRIPTOR_SET_ATTRIB_SAMPLER, .sampler = renderer->texSampler, .tex2D = depthOutput};
    VkuDescriptorSetAttribute ppSetAttribs[] = {ppSetAttrib1, ppSetAttrib2};
    VkuDescriptorSetCreateInfo ppDescriptorSetCreateInfo = {
        .attributes = ppSetAttribs,
        .attributeCount = 2,
        .renderStage = ppRenderStage};

    VkuDescriptorSet ppDescriptorSet = vkuCreateDescriptorSet(&ppDescriptorSetCreateInfo);

    VkuVertexAttribute ppVertexAttribs[] = {};
    VkuVertexLayout ppVertexLayout = {.attributeCount = 0, .attributes = ppVertexAttribs, .vertexSize = 0 * sizeof(float)};
    VkuPipelineCreateInfo ppPipelineInfo = {
        .vertexShaderSpirV = vkuReadFile("./shader/tjunctionPP_vert.spv", &ppPipelineInfo.vertexShaderLength),
        .fragmentShaderSpirV = vkuReadFile("./shader/tjunctionPP_frag.spv", &ppPipelineInfo.fragmentShaderLength),
        .vertexLayout = ppVertexLayout,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .descriptorSet = ppDescriptorSet,
        .depthTestWrite = VK_FALSE,
        .depthTestEnable = VK_FALSE,
        .depthCompareMode = VK_COMPARE_OP_LESS,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .renderStage = ppRenderStage};

    VkuPipeline ppPipeline = vkuCreatePipeline(renderer->context, &ppPipelineInfo);

    free(ppPipelineInfo.fragmentShaderSpirV);
    free(ppPipelineInfo.vertexShaderSpirV);

    SkyBox skybox = createSkyBox(renderer, renderer->blockRenderStage);

    float chunkDebugMeshVertices[216] = {};
    float subchunkDebugMeshVertices[216] = {};
    generateCubeTriangles(SC_LEN, SC_LEN * SC_CNT, SC_LEN, 0.0f, 1.0f, 0.0f, 0.0f, chunkDebugMeshVertices);
    generateCubeTriangles(SC_LEN, SC_LEN, SC_LEN, 1.0f, 0.0f, 0.0f, 1.0f, subchunkDebugMeshVertices);

    VkuBuffer stagingBuffer = vkuCreateVertexBuffer(renderer->context->memoryManager, sizeof(chunkDebugMeshVertices), VKU_BUFFER_USAGE_CPU_TO_GPU);

    vkuSetVertexBufferData(renderer->context->memoryManager, stagingBuffer, chunkDebugMeshVertices, sizeof(chunkDebugMeshVertices));
    renderer->chunkDebugMesh = vkuCreateVertexBuffer(renderer->context->memoryManager, sizeof(chunkDebugMeshVertices), VKU_BUFFER_USAGE_GPU_ONLY);
    VkDeviceSize sizes[] = {sizeof(chunkDebugMeshVertices)};
    vkuCopyBuffer(renderer->context->memoryManager, &stagingBuffer, &renderer->chunkDebugMesh, sizes, 1);

    vkuSetVertexBufferData(renderer->context->memoryManager, stagingBuffer, subchunkDebugMeshVertices, sizeof(subchunkDebugMeshVertices));
    renderer->subchunkDebugMesh = vkuCreateVertexBuffer(renderer->context->memoryManager, sizeof(subchunkDebugMeshVertices), VKU_BUFFER_USAGE_GPU_ONLY);
    vkuCopyBuffer(renderer->context->memoryManager, &stagingBuffer, &renderer->subchunkDebugMesh, sizes, 1);

    vkuDestroyVertexBuffer(stagingBuffer, renderer->context->memoryManager, VK_TRUE);

    VkuVertexAttribute debugAttrib1 = {.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0};
    VkuVertexAttribute debugAttrib2 = {.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(float) * 3};
    VkuVertexAttribute debugVertexAttribs[] = {debugAttrib1, debugAttrib2};
    VkuVertexLayout debugVertexLayout = {.attributeCount = 2, .attributes = debugVertexAttribs, .vertexSize = 6 * sizeof(float)};
    VkuPipelineCreateInfo debugPipelineInfo = {
        .fragmentShaderSpirV = vkuReadFile("./shader/debug_box_frag.spv", &debugPipelineInfo.fragmentShaderLength),
        .vertexShaderSpirV = vkuReadFile("./shader/debug_box_vert.spv", &debugPipelineInfo.vertexShaderLength),
        .polygonMode = VK_POLYGON_MODE_LINE,
        .descriptorSet = NULL,
        .vertexLayout = debugVertexLayout,
        .depthTestEnable = VK_TRUE,
        .depthTestWrite = VK_TRUE,
        .depthCompareMode = VK_COMPARE_OP_LESS,
        .renderStage = renderer->blockRenderStage,
        .cullMode = VK_CULL_MODE_BACK_BIT};

    renderer->debugPipeline = vkuCreatePipeline(renderer->context, &debugPipelineInfo);

    free(debugPipelineInfo.fragmentShaderSpirV);
    free(debugPipelineInfo.vertexShaderSpirV);

    pthread_mutex_init(&renderer->swapRendCksMutex, NULL);

    pthread_mutex_lock(&renderer->presenterMutex);
    renderer->presenterCreated = 1;
    pthread_cond_signal(&renderer->presenterCondition);
    pthread_mutex_unlock(&renderer->presenterMutex); 

    while (!vkuWindowShouldClose(renderer->presenter->window)) 
    {
        vkuDestroyBuffersInDestructionQueue(renderer->context->memoryManager, renderer->presenter);

        pthread_mutex_lock(&renderer->stopMutex);
        if (renderer->stopRenderThread) {
            pthread_mutex_unlock(&renderer->stopMutex);
            break;
        }
        pthread_mutex_unlock(&renderer->stopMutex);

        double frameDelta = getFrameDeltaTime();
        renderer->camera->speed = frameDelta * 100.0f;
        rendererHandleInput(renderer);

        VkuFrame frame = vkuPresenterBeginFrame(renderer->presenter);
        vkuFrameBeginRenderStage(frame, renderer->blockRenderStage);

        updateCameraProjMat(renderer->camera, vkuWindowGetAspect(renderer->presenter->window));

        renderWorld(frame, renderer, renderer->camera->view, renderer->camera->proj);
        renderSkybox(frame, skybox, renderer->camera->view, renderer->camera->proj, (float) renderer->camera->camPos.y, getNormalizedWorldTime(renderer->game->world));

        vkuFrameFinishRenderStage(frame, renderer->blockRenderStage);

        vkuFrameBeginRenderStage(frame, ppRenderStage);
        vkuFrameBindPipeline(frame, ppPipeline);

        int width, height;
        glfwGetWindowSize(renderer->presenter->window->glfwWindow, &width, &height);

        vkuFramePipelinePushConstant(frame, ppPipeline, (void*) (vec2) {1.0f / (float) width, 1.0f / (float) height}, sizeof(vec2));
        vkuFrameDrawVoid(frame, 3);
        vkuFrameFinishRenderStage(frame, ppRenderStage);

        vkuPresenterSubmitFrame(frame);
    }

    pthread_mutex_lock(&renderer->presenterMutex);
    renderer->renderThreadFinished = 1;
    pthread_mutex_unlock(&renderer->presenterMutex);

    pthread_mutex_lock(&renderer->cleanupMutex);
    while (!renderer->cleanup) {
        pthread_cond_wait(&renderer->cleanupCondition, &renderer->cleanupMutex);
    }
    pthread_mutex_unlock(&renderer->cleanupMutex);
    
    pthread_mutex_destroy(&renderer->swapRendCksMutex);
    vkuDestroyPipeline(renderer->context, renderer->debugPipeline);
    vkuDestroyVertexBuffer(renderer->chunkDebugMesh, renderer->context->memoryManager, VK_TRUE);
    vkuDestroyVertexBuffer(renderer->subchunkDebugMesh, renderer->context->memoryManager, VK_TRUE);
    destroySkyBox(skybox, renderer);
    vkuDestroyPipeline(renderer->context, ppPipeline);
    vkuDestroyDescriptorSet(ppDescriptorSet);
    vkuDestroyRenderStage(ppRenderStage);
    vkuDestroyDescriptorSet(renderer->descriptorSet);
    vkuDestroyPipeline(renderer->context, renderer->blockPipeline);
    vkuDestroyUniformBuffer(renderer->context, renderer->sceneUniBuffer);
    vkuDestroyTextureSampler(renderer->context, renderer->texSampler);
    vkuDestroyTexture2DArray(renderer->context, renderer->blockArray);
    vkuDestroyRenderStage(renderer->blockRenderStage);
    destroyCamera(renderer->camera);
    destroyInputHandler(renderer->inputHandler);
    vkuDestroyBuffersInDestructionQueue(renderer->context->memoryManager, renderer->presenter);
    vkuDestroyPresenter(renderer->presenter);
    vkuDestroyContext(renderer->context);

    return NULL;
}

Renderer createRenderer(Registry registry, Game game)
{
    Renderer renderer = (Renderer) calloc(1, sizeof(Renderer_T));
    renderer->stopRenderThread = 0;
    renderer->presenterCreated = 0;
    renderer->renderThreadFinished = 0;
    renderer->cleanup = 0;
    renderer->registry = registry;
    renderer->game = game;

    pthread_mutex_init(&renderer->stopMutex, NULL);
    pthread_mutex_init(&renderer->presenterMutex, NULL);
    pthread_mutex_init(&renderer->cleanupMutex, NULL);
    pthread_cond_init(&renderer->presenterCondition, NULL);
    pthread_cond_init(&renderer->cleanupCondition, NULL);

    if (pthread_create(&renderer->renderThread, NULL, renderFunction, (void*) renderer) != 0)
        perror("Failed to create renderThread!\n");

    pthread_mutex_lock(&renderer->presenterMutex);
    while (!renderer->presenterCreated) {
        pthread_cond_wait(&renderer->presenterCondition, &renderer->presenterMutex);
    }
    pthread_mutex_unlock(&renderer->presenterMutex);

    return renderer;
}

void destroyRenderer(Renderer renderer)
{
    pthread_mutex_lock(&renderer->stopMutex);
    renderer->stopRenderThread = 1;
    pthread_mutex_unlock(&renderer->stopMutex);

    pthread_mutex_lock(&renderer->cleanupMutex);
    renderer->cleanup = 1;
    pthread_cond_signal(&renderer->cleanupCondition);
    pthread_mutex_unlock(&renderer->cleanupMutex);

    if (pthread_join(renderer->renderThread, NULL) != 0) {
        perror("Failed to join thread");
        return;
    }

    pthread_mutex_destroy(&renderer->stopMutex);
    pthread_mutex_destroy(&renderer->presenterMutex);
    pthread_mutex_destroy(&renderer->cleanupMutex);
    pthread_cond_destroy(&renderer->presenterCondition);
    pthread_cond_destroy(&renderer->cleanupCondition);

    free(renderer);
}

int RendererHasFinished(Renderer renderer) 
{
    int finished;

    pthread_mutex_lock(&renderer->presenterMutex);
    finished = renderer->renderThreadFinished;
    pthread_mutex_unlock(&renderer->presenterMutex);

    return finished;
}

// chunkMesh

chunkMesh createChunkMesh(Renderer renderer)
{
    chunkMesh mesh = (chunkMesh) calloc(1, sizeof(chunkMesh_T));
    pthread_mutex_init(&mesh->mutex, NULL);
    atomic_init(&mesh->swap_cond, false);
    return mesh;
}

void chunkMeshInit(Renderer renderer, chunkMesh chunkMesh, VkDeviceSize size, VkuBufferUsage usage)
{
    pthread_mutex_lock(&chunkMesh->mutex);

    if (chunkMesh->opaqueBuffer[MESH_FIRST] != NULL)
        vkuDestroyVertexBuffer(chunkMesh->opaqueBuffer[MESH_FIRST], renderer->context->memoryManager, VK_FALSE);

    chunkMesh->bufferSize[MESH_FIRST] = size;
    chunkMesh->opaqueBuffer[MESH_FIRST] = vkuCreateVertexBuffer(renderer->context->memoryManager, size, usage);

    pthread_mutex_unlock(&chunkMesh->mutex);
}

void chunkMeshCopy(Renderer renderer, chunkMesh *src, chunkMesh *dst, VkDeviceSize *sizes, uint32_t cnt)
{
    VkuBuffer srcBuffer[cnt];
    VkuBuffer dstBuffer[cnt];
    memset(srcBuffer, 0, sizeof(srcBuffer));
    memset(dstBuffer, 0, sizeof(dstBuffer));

    uint32_t validCount = 0;

    for (uint32_t i = 0; i < cnt; i++)
    {
        pthread_mutex_lock(&src[i]->mutex);
        
        if (src[i]->opaqueBuffer[MESH_FIRST] != NULL && dst[i]->opaqueBuffer[MESH_FIRST] != NULL)
        {
            srcBuffer[validCount] = src[i]->opaqueBuffer[MESH_FIRST];
            dstBuffer[validCount] = dst[i]->opaqueBuffer[MESH_FIRST];
            sizes[validCount] = sizes[i];
            validCount++;
        }
    }

    vkuCopyBuffer(renderer->context->memoryManager, srcBuffer, dstBuffer, sizes, validCount);

    for (uint32_t i = 0; i < cnt; i++)
    {
        pthread_mutex_unlock(&src[i]->mutex);
    }
}

void chunkMeshSwap(Renderer renderer, chunkMesh mesh)
{
    pthread_mutex_lock(&mesh->mutex);

    if (atomic_load(&mesh->swap_cond))
    {
        pthread_mutex_unlock(&mesh->mutex);
        return;
    }

    if (mesh->opaqueBuffer[MESH_SECOND] != NULL)
    {
        vkuEnqueueBufferDestruction(renderer->context->memoryManager, mesh->opaqueBuffer[MESH_SECOND]);
        mesh->opaqueBuffer[MESH_SECOND] = NULL;
    }

    mesh->opaqueBuffer[MESH_SECOND] = mesh->opaqueBuffer[MESH_FIRST];
    mesh->opaqueBuffer[MESH_FIRST] = NULL;

    mesh->bufferSize[MESH_SECOND] = mesh->bufferSize[MESH_FIRST];
    mesh->bufferSize[MESH_FIRST] = 0;

    pthread_mutex_unlock(&mesh->mutex);
}

float * mapChunkMesh(Renderer renderer, chunkMesh mesh)
{   
    pthread_mutex_lock(&mesh->mutex);
    float *mapped = NULL;
    if (mesh->opaqueBuffer[MESH_FIRST] != NULL)
        mapped = (float*) vkuMapVertexBuffer(renderer->context->memoryManager, mesh->opaqueBuffer[MESH_FIRST]);
    pthread_mutex_unlock(&mesh->mutex);
    return mapped;
}

void unmapChunkMesh(Renderer renderer, chunkMesh mesh)
{
    pthread_mutex_lock(&mesh->mutex);
    if (mesh->opaqueBuffer[MESH_FIRST] != NULL)
        vkuUnmapVertexBuffer(renderer->context->memoryManager, mesh->opaqueBuffer[MESH_FIRST]);
    pthread_mutex_unlock(&mesh->mutex);
}

void destroyChunkMesh(Renderer renderer, chunkMesh chunkMesh)
{
    pthread_mutex_lock(&chunkMesh->mutex);

    if (chunkMesh->opaqueBuffer[MESH_FIRST] != NULL) {
        vkuEnqueueBufferDestruction(renderer->context->memoryManager, chunkMesh->opaqueBuffer[MESH_FIRST]);
    }
        
    if (chunkMesh->opaqueBuffer[MESH_SECOND] != NULL) 
        vkuEnqueueBufferDestruction(renderer->context->memoryManager, chunkMesh->opaqueBuffer[MESH_SECOND]);

    pthread_mutex_unlock(&chunkMesh->mutex);
    pthread_mutex_destroy(&chunkMesh->mutex);
    free(chunkMesh);
}

bool isChunkMeshReadyForSwap(chunkMesh mesh) {
    if (pthread_mutex_trylock(&mesh->mutex) == 0) {
        bool ready = (mesh->opaqueBuffer[MESH_FIRST] != NULL) && !atomic_load(&mesh->swap_cond);
        pthread_mutex_unlock(&mesh->mutex);

        return ready;
    } else {
        return false;
    }
}

void chunkMeshResize(Renderer renderer, chunkMesh mesh, VkDeviceSize newSize, VkuBufferUsage usage)
{
    pthread_mutex_lock(&mesh->mutex);

    if (mesh->opaqueBuffer[MESH_SECOND] != NULL) {
        pthread_mutex_unlock(&mesh->mutex);  // <-- unlock before returning
        return;
    }

    VkuBuffer tempBuffer = vkuCreateVertexBuffer(renderer->context->memoryManager, newSize, usage);
    vkuCopyBuffer(renderer->context->memoryManager, &(mesh->opaqueBuffer[MESH_FIRST]), &tempBuffer, mesh->bufferSize, 1);

    vkuDestroyVertexBuffer(mesh->opaqueBuffer[MESH_FIRST], renderer->context->memoryManager, VK_FALSE);

    mesh->opaqueBuffer[MESH_FIRST] = tempBuffer;
    mesh->bufferSize[MESH_FIRST]   = newSize;

    pthread_mutex_unlock(&mesh->mutex);
}