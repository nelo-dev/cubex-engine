#include "light.h"

#include <unistd.h>

// LightEngine

static inline uint32_t getMaxY(const Chunk ori, const Chunk PX, const Chunk NX, const Chunk PZ, const Chunk NZ, uint32_t x, uint32_t z) {
    uint32_t maxY = ori->heightMap[x + z * SC_LEN] + 1;

    if (x < SC_LEN - 1) {
        maxY = (ori->heightMap[x + 1 + z * SC_LEN] > maxY) ? ori->heightMap[x + 1 + z * SC_LEN] : maxY;
    } else {
        maxY = (PX->heightMap[z * SC_LEN] > maxY) ? PX->heightMap[z * SC_LEN] : maxY;
    }

    if (x > 0) {
        maxY = (ori->heightMap[x - 1 + z * SC_LEN] > maxY) ? ori->heightMap[x - 1 + z * SC_LEN] : maxY;
    } else {
        maxY = (NX->heightMap[z * SC_LEN + (SC_LEN - 1)] > maxY) ? NX->heightMap[z * SC_LEN + (SC_LEN - 1)] : maxY;
    }

    if (z < SC_LEN - 1) {
        maxY = (ori->heightMap[x + (z + 1) * SC_LEN] > maxY) ? ori->heightMap[x + (z + 1) * SC_LEN] : maxY;
    } else {
        maxY = (PZ->heightMap[x] > maxY) ? PZ->heightMap[x] : maxY;
    }

    if (z > 0) {
        maxY = (ori->heightMap[x + (z - 1) * SC_LEN] > maxY) ? ori->heightMap[x + (z - 1) * SC_LEN] : maxY;
    } else {
        maxY = (NZ->heightMap[x + (SC_LEN - 1) * SC_LEN] > maxY) ? NZ->heightMap[x + (SC_LEN - 1) * SC_LEN] : maxY;
    }

    return maxY;
}

int getBlockAndLight(Chunk chunks[9], int16_t x, int16_t y, int16_t z, Block **blk, LightValue **lv) {
    x += SC_LEN;
    z += SC_LEN;
    int chunkX = x >> 5, chunkZ = z >> 5;
    int chunkIdx = chunkZ * 3 + chunkX;

    if (y < 0 || y >= SC_CNT * SC_LEN) return -1; 
    if (!chunks[chunkIdx]->subchunks[y >> 5]) return -2;

    Subchunk subchunk = chunks[chunkIdx]->subchunks[y >> 5];
    int blockIdx = ((y & 31) << 10) + ((z & 31) << 5) + (x & 31);

    *blk = &subchunk->blocks[blockIdx];
    *lv = &subchunk->light[blockIdx];
    return 0;
}

static void propagateSunlight(Chunk chunks[9], lightEngine engine) {
    FifoQueue *queue = engine->sunlightPropagationQueue;
    Block *blk;
    LightValue *lv;
    Block *tempblk;
    LightValue *templv;

    static const int offsets[4][2] = {{1, 0}, {-1, 0}, {0, -1}, {0, 1}};

    while (!fifo_queue_is_empty(queue)) {
        lightNode node;
        fifo_queue_dequeue(queue, &node);
        int16_t light = node.light;
        int y = node.y;
        int newLightLevel = 0;
        
        if (node.hrz_spread && getBlockAndLight(chunks, node.x, y + 1, node.z, &blk, &lv) == 0) {
            newLightLevel = light - SUNLIGHT_HORI_MINUS - engine->blockOpacities[*blk];

            if (newLightLevel > GET_SUNLIGHT((*lv))) {
                lightNode sideNode = {.light = newLightLevel, .x = node.x, .z = node.z, .y = y + 1, .hrz_spread = true};
                fifo_queue_enqueue(queue, (void*)&sideNode);
                SET_SUNLIGHT((*lv), newLightLevel);
            }
        }

        for (int i = 0; i < 4; ++i) {
            int16_t newX = node.x + offsets[i][0], newZ = node.z + offsets[i][1];

            if (getBlockAndLight(chunks, newX, y, newZ, &blk, &lv) == 0) {
                newLightLevel = light - SUNLIGHT_HORI_MINUS - engine->blockOpacities[*blk];
                if (newLightLevel > GET_SUNLIGHT((*lv)) && getBlockAndLight(chunks, newX, y + 1, newZ, &tempblk, &templv) == 0 && engine->blockOpacities[*tempblk] != TRANSPARENT) {
                    lightNode sideNode = {.light = newLightLevel, .x = newX, .z = newZ, .y = y, .hrz_spread = true};
                    fifo_queue_enqueue((queue), (void*) &sideNode);
                    SET_SUNLIGHT((*lv), newLightLevel);
                }
            }
        }

        y--;

        while (y >= 0) { 
            int res = getBlockAndLight(chunks, node.x, y, node.z, &blk, &lv);
            if (res == 0) {
                light = light - engine->blockOpacities[*blk];
                if (light > GET_SUNLIGHT((*lv))) {
                    SET_SUNLIGHT((*lv), light);

                    for (int i = 0; i < 4; ++i) {
                        int16_t newX = node.x + offsets[i][0], newZ = node.z + offsets[i][1];

                        if (getBlockAndLight(chunks, newX, y, newZ, &blk, &lv) == 0) {
                            newLightLevel = light - SUNLIGHT_HORI_MINUS - engine->blockOpacities[*blk];
                            if (newLightLevel > GET_SUNLIGHT((*lv)) && getBlockAndLight(chunks, newX, y + 1, newZ, &tempblk, &templv) == 0 && engine->blockOpacities[*tempblk] != TRANSPARENT) {
                                lightNode sideNode = {.light = newLightLevel, .x = newX, .z = newZ, .y = y, .hrz_spread = true};
                                fifo_queue_enqueue((queue), (void*) &sideNode);
                                SET_SUNLIGHT((*lv), newLightLevel);
                            }
                        }
                    }
                } else break;

                y--;
            } else if (res == -2) {
                if ((node.x == 0 && chunks[CK_NX]->subchunks[y >> 5]) || (node.x == SC_LEN - 1  && chunks[CK_PX]->subchunks[y >> 5]) || (node.z == 0  && chunks[CK_NZ]->subchunks[y >> 5]) || (node.z == SC_LEN - 1 && chunks[CK_PZ]->subchunks[y >> 5]) ||
                    (chunks[CK_ORI]->subchunks[y >> 5] && ((node.x == -1 ) || (node.x == SC_LEN) || (node.z == -1) || (node.z == SC_LEN)))) {
                    for (int i = 0; i < 4; ++i) {
                        int16_t newX = node.x + offsets[i][0], newZ = node.z + offsets[i][1];

                        if (getBlockAndLight(chunks, newX, y, newZ, &blk, &lv) == 0) {
                            newLightLevel = light - SUNLIGHT_HORI_MINUS - engine->blockOpacities[*blk];
                            if (newLightLevel > GET_SUNLIGHT((*lv)) && getBlockAndLight(chunks, newX, y + 1, newZ, &tempblk, &templv) == 0 && engine->blockOpacities[*tempblk] != TRANSPARENT) {
                                lightNode sideNode = {.light = newLightLevel, .x = newX, .z = newZ, .y = y, .hrz_spread = true};
                                fifo_queue_enqueue((queue), (void*) &sideNode);
                                SET_SUNLIGHT((*lv), newLightLevel);
                            }
                        }
                    }

                    y -= 1;
                } else {
                    y -= SC_LEN;
                }
            } else {
                break;
            }
        }
    }
}

static void initSunlight(Chunk chunks[9], lightEngine engine) {
    FifoQueue *queue = engine->sunlightPropagationQueue;
    Chunk ori = chunks[CK_ORI];
    Chunk PX = chunks[CK_PX], NX = chunks[CK_NX], PZ = chunks[CK_PZ], NZ = chunks[CK_NZ];

    for (uint32_t z = 0; z < SC_LEN; z++) {
        for (uint32_t x = 0; x < SC_LEN; x++) {
            uint32_t y = getMaxY(ori, PX, NX, PZ, NZ, x, z);
            fifo_queue_enqueue((queue), (void*) &(lightNode)  {.light = UINT4_MAX, .x = (int16_t)x, .z = (int16_t)z, .y = (int16_t)y, .hrz_spread = false});

            Block *blk;
            LightValue *lv;

            if (getBlockAndLight(chunks, x, y, z, &blk, &lv) == 0) {
                if (UINT4_MAX > GET_SUNLIGHT((*lv))) {
                    SET_SUNLIGHT((*lv), UINT4_MAX);
                }
            }
        }
    }

    propagateSunlight(chunks, engine);
}

static void initAdjacentSunlight(Chunk chunks[9], lightEngine engine) {
    FifoQueue *queue = engine->sunlightPropagationQueue;

    
}

void *lightingFunction(void *arg)
{
    lightEngine engine = (lightEngine)arg;

    while (1)
    {
        if (ts_fifo_queue_count(&engine->queue) == 0)
            atomic_store(&engine->idle, true);

        lightingSubmitInfo info;
        int res = lightingDequeue(&engine->queue, &info);
        atomic_store(&engine->idle, false);

        if (res == -1) {
            continue;
        } else if (res == 1) {
            break;
        }

        if (!atomic_load(&info.chunks[CK_ORI]->isRendered))
            continue;

        lock_chunks(info.chunks, CK_POS_CNT);

        if (info.init)
        {   
            initSunlight(info.chunks, engine);
            initAdjacentSunlight(info.chunks, engine);

            BlockChangeInfo item;
            while (ts_fifo_queue_try_dequeue(&info.chunks[CK_ORI]->blockChangeQueue, &item) != TS_FIFO_QUEUE_ERROR);

            setLightingState(&info.chunks[CK_ORI]->lightingState, LIGHT_STATE_LIT);

            for (uint32_t i = 0; i < 9; i++) {
                LightingState state = getLightingState(&info.chunks[i]->lightingState);
                if (state == LIGHT_STATE_LIT) {
                    setMeshingState(&info.chunks[i]->meshingState, MESH_STATE_NEEDS_MESHING);
                }
            }

            info.chunks[CK_PX]->lightByChunks[CHUNK_NX] = true;
            info.chunks[CK_NX]->lightByChunks[CHUNK_PX] = true;
            info.chunks[CK_PZ]->lightByChunks[CHUNK_NZ] = true;
            info.chunks[CK_NZ]->lightByChunks[CHUNK_PZ] = true;
            
            unlock_chunks(info.chunks, CK_POS_CNT);

            continue;
        }

        static const int16_t offsets[6][4] = {
            { 1,  0,  0, SUNLIGHT_HORI_MINUS},
            {-1,  0,  0, SUNLIGHT_HORI_MINUS},
            { 0,  1,  0, 0},
            { 0, -1,  0, SUNLIGHT_HORI_MINUS},
            { 0,  0,  1, SUNLIGHT_HORI_MINUS},
            { 0,  0, -1, SUNLIGHT_HORI_MINUS}
        };

        BlockChangeInfo item;
        while (ts_fifo_queue_try_dequeue(&info.chunks[CK_ORI]->blockChangeQueue, &item) != TS_FIFO_QUEUE_ERROR) {
            Block *blk;
            LightValue *lv;

            if (getBlockAndLight(info.chunks, item.x, item.y, item.z, &blk, &lv) != 0) {
                continue;
            }

            int currentLight = GET_SUNLIGHT((*lv));
            int maxNewLight = currentLight;

            for (uint32_t i = 0; i < 6; i++) {
                int16_t nx = item.x + offsets[i][0];
                int16_t ny = item.y + offsets[i][1];
                int16_t nz = item.z + offsets[i][2];

                if (ny == SC_CNT * SC_LEN) {
                    maxNewLight = UINT4_MAX;
                    break;
                }

                Block *scanBlk;
                LightValue *scanLv;
                if (getBlockAndLight(info.chunks, nx, ny, nz, &scanBlk, &scanLv) == 0) {
                    int candidate = GET_SUNLIGHT((*scanLv)) - offsets[i][3] - engine->blockOpacities[*blk];
                    if (candidate > maxNewLight) {
                        maxNewLight = candidate;
                    }
                }

                if (maxNewLight == UINT4_MAX) break;
            }

            if (maxNewLight > currentLight) {
                SET_SUNLIGHT((*lv), maxNewLight);
                fifo_queue_enqueue(engine->sunlightPropagationQueue,
                    (void*) &(lightNode){.hrz_spread = true, .light = maxNewLight, .x = item.x, .y = item.y, .z = item.z});
            }
        }

        propagateSunlight(info.chunks, engine);
        
        setLightingState(&info.chunks[CK_ORI]->lightingState, LIGHT_STATE_LIT);

        for (uint32_t i = 0; i < 9; i++) {
            LightingState state = getLightingState(&info.chunks[i]->lightingState);
            if (state == LIGHT_STATE_LIT) {
                setMeshingState(&info.chunks[i]->meshingState, MESH_STATE_NEEDS_MESHING);
            }
        }

        unlock_chunks(info.chunks, CK_POS_CNT);
    }

    return NULL;
}

void createLightingQueue(TS_FIFO_Queue * lightingQueue) 
{
    ts_fifo_queue_init(lightingQueue, sizeof(lightingSubmitInfo), INITIAL_LIGHTING_QUEUE_CAP);
}

void lightingEnqueue(TS_FIFO_Queue * lightingQueue, lightingSubmitInfo *info) 
{
    setLightingState(&info->chunks[CK_ORI]->lightingState, LIGHT_STATE_QUEUED);
    ts_fifo_queue_enqueue(lightingQueue, info);
}

int lightingDequeue(TS_FIFO_Queue * lightingQueue, lightingSubmitInfo *info) 
{
    if (!info) return -1;

    if (ts_fifo_queue_dequeue(lightingQueue, info) != 0) {
        return -1;
    }

    if (info->stopState == 1) {
        return 1;
    }

    if (getLightingState(&info->chunks[CK_ORI]->lightingState) != LIGHT_STATE_QUEUED) {
        return -1;
    }

    return 0;
}

void destroyLightingQueue(TS_FIFO_Queue * lightQueue) 
{
    if (lightQueue) {
        ts_fifo_queue_destroy(lightQueue);
    }
}

lightEngine createLightEngine(Registry registry, Mesher mesher) 
{
    lightEngine_T *eng = malloc(sizeof(lightEngine_T));
    eng->mesher = mesher;
    eng->sunlightPropagationQueue = fifo_queue_create(sizeof(lightNode), SC_LEN);
    eng->registry = registry;
    createLightingQueue(&eng->queue);

    eng->blockOpacities = malloc(sizeof(uint16_t) * eng->registry->blockList->infoArray.size);
    eng->blockEmissions = malloc(sizeof(uint16_t) * eng->registry->blockList->infoArray.size);

    BlockInfo *bkInfos = (BlockInfo *) eng->registry->blockList->infoArray.data;

    for (uint32_t i = 0; i < eng->registry->blockList->infoArray.size; i++) {
        eng->blockEmissions[i] = (uint16_t) bkInfos[i].emittedLight;
        eng->blockOpacities[i] = (uint16_t) bkInfos[i].opacity;
    }

    if (pthread_create(&eng->thread, NULL, lightingFunction, (void*) eng) != 0)
    {
        destroyLightEngine(eng);
        return NULL;
    }

    return eng;
}

void destroyLightEngine(lightEngine lightEngine) 
{
    lightingSubmitInfo sentinel = {.chunks = NULL, .stopState = 1};
    ts_fifo_queue_enqueue(&lightEngine->queue, &sentinel);

    pthread_join(lightEngine->thread, NULL); 

    free(lightEngine->blockEmissions);
    free(lightEngine->blockOpacities);

    destroyLightingQueue(&lightEngine->queue);
    fifo_queue_destroy(lightEngine->sunlightPropagationQueue);
    free(lightEngine);
}

// ThreadSafeLightState

void initThreadSafeLightingState(ThreadSafeLightingState *lightingState, LightingState initialState) 
{
    lightingState->state = initialState;
    pthread_mutex_init(&lightingState->mutex, NULL);
}

void setLightingState(ThreadSafeLightingState *lightingState, LightingState newState) 
{
    pthread_mutex_lock(&lightingState->mutex);

    if (lightingState->state == LIGHT_STATE_UNLIT && newState == LIGHT_STATE_NEEDS_LIGHTING)
    {
        pthread_mutex_unlock(&lightingState->mutex);
        return;
    }

    if (lightingState->state == LIGHT_STATE_QUEUED && newState == LIGHT_STATE_NEEDS_LIGHTING)
    {
        pthread_mutex_unlock(&lightingState->mutex);
        return;
    }

    lightingState->state = newState;
    pthread_mutex_unlock(&lightingState->mutex);
}

LightingState getLightingState(ThreadSafeLightingState *lightingState) 
{
    LightingState currentState;
    pthread_mutex_lock(&lightingState->mutex);
    currentState = lightingState->state;
    pthread_mutex_unlock(&lightingState->mutex);
    return currentState;
}

void destroyThreadSafeLightingState(ThreadSafeLightingState *lightingState) 
{
    pthread_mutex_destroy(&lightingState->mutex);
}