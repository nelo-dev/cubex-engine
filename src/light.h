#ifndef LIGHT_H
#define LIGHT_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

#define SUNLIGHT_HORI_MINUS 1

typedef enum chunkPosition {
    CK_NXNZ,
    CK_NZ,
    CK_PXNZ,
    CK_NX,
    CK_ORI,
    CK_PX,
    CK_NXPZ,
    CK_PZ,
    CK_PXPZ,
    CK_POS_CNT
} chunkPosition;

typedef union {
    uint8_t value;
    struct {
        uint8_t blocklight : 4;
        uint8_t sunlight : 4;
    };
} LightValue;

#define SET_SUNLIGHT(lightValue, sun) \
    (lightValue.value = (lightValue.value & 0x0F) | ((sun & 0x0F) << 4))

#define GET_SUNLIGHT(lightValue) \
    ((lightValue.value >> 4) & 0x0F)

#define SET_BLOCKLIGHT(lightValue, block) \
    (lightValue.value = (lightValue.value & 0xF0) | (block & 0x0F))

#define GET_BLOCKLIGHT(lightValue) \
    (lightValue.value & 0x0F)

#define GET_COMPLETE_LIGHT(lightValue) \
    (lightValue.value)

typedef enum LightingState {
    LIGHT_STATE_UNLIT,
    LIGHT_STATE_QUEUED,
    LIGHT_STATE_NEEDS_LIGHTING,
    LIGHT_STATE_LIT
} LightingState;

typedef struct ThreadSafeLightingState {
    LightingState state;
    pthread_mutex_t mutex;
} ThreadSafeLightingState;

void initThreadSafeLightingState(ThreadSafeLightingState *lightingState, LightingState initialState);
void setLightingState(ThreadSafeLightingState *lightingState, LightingState newState);
LightingState getLightingState(ThreadSafeLightingState *lightingState);
void destroyThreadSafeLightingState(ThreadSafeLightingState *lightingState);

#include "../lib/datastrucs/fifoqueue.h"
#include "registry.h"
#include "chunk.h"
#include "mesher.h"

typedef struct lightingSubmitInfo
{
    Chunk chunks[CK_POS_CNT];
    bool stopState;
    bool init;
} lightingSubmitInfo;

void createLightingQueue(TS_FIFO_Queue * lightingQueue);
void lightingEnqueue(TS_FIFO_Queue * lightingQueue, lightingSubmitInfo *info);
int lightingDequeue(TS_FIFO_Queue * lightingQueue, lightingSubmitInfo *info);
void destroyLightingQueue(TS_FIFO_Queue * meshQueue);

typedef struct Mesher_T * Mesher;

typedef struct lightNode {
    int16_t x;
    int16_t z;
    int16_t y;
    int8_t light;
    bool hrz_spread;
} lightNode;

#define INITIAL_LIGHTING_QUEUE_CAP  1024

typedef struct lightEngine_T {
    Registry registry;
    Mesher mesher;

    FifoQueue * sunlightPropagationQueue;
    pthread_t thread;
    TS_FIFO_Queue queue;
    atomic_bool idle;

    uint16_t * blockOpacities;
    uint16_t * blockEmissions;
} lightEngine_T;

typedef lightEngine_T * lightEngine;

lightEngine createLightEngine(Registry registry, Mesher mesher);
void destroyLightEngine(lightEngine lightEngine);

#endif