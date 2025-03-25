#ifndef RENDERER_H
#define RENDERER_H

#include "../lib/vkutils/src/vkutils.h"
#include "input.h"
#include "camera.h"
#include "skybox.h"
#include "registry.h"

typedef struct Game_T *Game;

#define MIP_LEVELS  4

typedef struct Renderer_T {
    VkuContext context;
    VkuPresenter presenter;
    InputHandler inputHandler; // Use forward-declared type
    Camera camera;
    Registry registry;
    Game game;

    VkuRenderStage blockRenderStage;
    VkuTexture2DArray blockArray;
    VkuUniformBuffer sceneUniBuffer;
    VkuTextureSampler texSampler;
    VkuDescriptorSet descriptorSet;
    VkuPipeline blockPipeline;
    pthread_mutex_t swapRendCksMutex;

    pthread_t renderThread;
    pthread_mutex_t stopMutex;
    pthread_mutex_t presenterMutex;
    pthread_mutex_t cleanupMutex;
    pthread_cond_t presenterCondition;
    pthread_cond_t cleanupCondition;
    int stopRenderThread;
    int presenterCreated;
    int renderThreadFinished;
    int cleanup;
} Renderer_T;

typedef Renderer_T *Renderer;

typedef struct {
    float sunlight;

    vec4 fogColor;
    vec3 camPos;
    float start;
    float end;
    float density;
} block_ubo;

void rendererHandleInput(Renderer renderer);
void * renderFunction(void * arg);
Renderer createRenderer(Registry registry, Game game);
void destroyRenderer(Renderer renderer);
int RendererHasFinished(Renderer renderer);

typedef enum bufferIndex {
    MESH_FIRST,
    MESH_SECOND,
    MESH_CNT
} bufferIndex;

typedef struct chunkMesh_T
{
    VkuBuffer opaqueBuffer[MESH_CNT];
    VkDeviceSize bufferSize[MESH_CNT];
    pthread_mutex_t mutex;
    atomic_bool swap_cond;
} chunkMesh_T;

typedef chunkMesh_T * chunkMesh;

chunkMesh createChunkMesh(Renderer renderer);
void destroyChunkMesh(Renderer renderer, chunkMesh chunkMesh);
void chunkMeshInit(Renderer renderer, chunkMesh chunkMesh, VkDeviceSize size, VkuBufferUsage usage);
void chunkMeshCopy(Renderer renderer, chunkMesh *src, chunkMesh *dst, VkDeviceSize *sizes, uint32_t cnt);
void chunkMeshSwap(Renderer renderer, chunkMesh mesh);
float * mapChunkMesh(Renderer renderer, chunkMesh mesh);
void unmapChunkMesh(Renderer renderer, chunkMesh mesh);
bool isChunkMeshReadyForSwap(chunkMesh mesh);
void chunkMeshResize(Renderer renderer, chunkMesh mesh, VkDeviceSize newSize, VkuBufferUsage usage);

#endif