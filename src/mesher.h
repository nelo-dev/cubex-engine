#ifndef MESHER
#define MESHER

#include <pthread.h>
#include <stdbool.h>

// chunkMesh

typedef enum MeshingState {
    MESH_STATE_UNMESHED,
    MESH_STATE_QUEUED,
    MESH_STATE_NEEDS_MESHING,
    MESH_STATE_IN_PROGRESS,
    MESH_STATE_MESHED
} MeshingState;

typedef struct ThreadSafeMeshingState {
    MeshingState state;
    pthread_mutex_t mutex;
} ThreadSafeMeshingState;

void initThreadSafeMeshingState(ThreadSafeMeshingState *meshingState, MeshingState initialState);
void setMeshingState(ThreadSafeMeshingState *meshingState, MeshingState newState);
MeshingState getMeshingState(ThreadSafeMeshingState *meshingState);
void destroyThreadSafeMeshingState(ThreadSafeMeshingState *meshingState);

// Mesher

#include "chunk.h"
#include "registry.h"
#include "renderer.h"
#include "light.h"
#include "../lib/datastrucs/ts_fifo.h"

#define UINT4_MAX                           15
#define INITIAL_MESH_QUEUE_CAP              1024
#define VERTEX_FLOAT_COUNT                  3
#define INITIAL_MESHING_VERTEX_BUFFER_SIZE  sizeof(float) * VERTEX_FLOAT_COUNT * 1024
#define BATCHED_UPLOAD_BUFFER_CNT           12
#define NORMAL_BLOCK_REQUIRED_MEMORY        6 * 6 * VERTEX_FLOAT_COUNT * sizeof(float)

#define CHUNK_PX 0
#define CHUNK_NX 1
#define CHUNK_PZ 2
#define CHUNK_NZ 3

#define SUBCHUNK_PX 0
#define SUBCHUNK_NX 1
#define SUBCHUNK_PY 2
#define SUBCHUNK_NY 3
#define SUBCHUNK_PZ 4
#define SUBCHUNK_NZ 5

#define FACE_PX 0
#define FACE_NX 1
#define FACE_PY 2
#define FACE_NY 3
#define FACE_PZ 4
#define FACE_NZ 5

#define LIGHT_EXP_PX    7
#define LIGHT_EXP_NX    7
#define LIGHT_EXP_PY    0
#define LIGHT_EXP_NY    10
#define LIGHT_EXP_PZ    2
#define LIGHT_EXP_NZ    2

#define SET_BIT(byte, position) ((byte) |= (1 << (position)))
#define CLEAR_BIT(byte, position) ((byte) &= ~(1 << (position)))
#define GET_BIT(byte, position) (((byte) >> (position)) & 1)
#define SET_BIT_VALUE(byte, position, value) ((value) ? SET_BIT(byte, position) : CLEAR_BIT(byte, position))

typedef struct mesherSubmitInfo
{
    Chunk chunk;
    Chunk sidechunks[4];
    bool stopState;
} mesherSubmitInfo;

typedef struct Mesher_T
{
    Renderer renderer;
    Registry registry;
    TS_FIFO_Queue queue;
    Subchunk airSubchunk;
    uint8_t greedyBuffer[SC_VOL];

    VkDeviceSize bufferSizes[BATCHED_UPLOAD_BUFFER_CNT];
    chunkMesh stagingbuffers[BATCHED_UPLOAD_BUFFER_CNT];
    float * mappedBuffers[BATCHED_UPLOAD_BUFFER_CNT];

    pthread_t thread;
    atomic_bool idle;
} Mesher_T;

typedef struct Mesher_T * Mesher;

Mesher createMesher(Registry registry, Renderer renderer);
void destroyMesher(Mesher mesher);
void mesherEnqueue(TS_FIFO_Queue * meshQueue, mesherSubmitInfo *info);

#endif