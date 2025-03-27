#include "chunk.h"
#include <stdlib.h>

#define INDEX(x, y, z) ((x) + ((y) * SC_PLN) + ((z) * SC_LEN))
#define SC_INDEX(y) (y / SC_LEN)

Subchunk createSubchunk()
{
    Subchunk_T *subchunk = calloc(1, sizeof(Subchunk_T));
    return subchunk;
}

void destroySubchunk(Subchunk subchunk)
{
    if (subchunk != NULL)
        free(subchunk);
}

Chunk createChunk(int32_t chunkX, int32_t chunkZ, Renderer renderer)
{
    Chunk_T *chunk = calloc(1, sizeof(Chunk_T));
    chunk->chunkX = chunkX;
    chunk->chunkZ = chunkZ;
    chunk->renderer = renderer;

    for (uint32_t i = 0; i < SC_CNT; i++)
        chunk->subchunks[i] = NULL;

    initThreadSafeMeshingState(&chunk->meshingState, MESH_STATE_UNMESHED);
    initThreadSafeLightingState(&chunk->lightingState, LIGHT_STATE_UNLIT);
    ts_fifo_queue_init(&chunk->blockChangeQueue, sizeof(BlockChangeInfo), 64);
    chunk->chunkMesh = createChunkMesh(renderer);

    atomic_store(&chunk->created, true);
    atomic_store(&chunk->isRendered, false);
    atomic_store(&chunk->queuedForDestruction, false);
    pthread_mutex_init(&chunk->lockMutex, NULL);
    return chunk;
}

bool isChunkCreated(Chunk chunk)
{
    if (chunk == NULL)
        return false;
}

void destroyChunk(Chunk chunk)
{
    if (chunk == NULL)
        return;

    for (uint32_t i = 0; i < SC_CNT; i++)
        if (chunk->subchunks[i] != NULL)
            destroySubchunk(chunk->subchunks[i]);

    pthread_mutex_destroy(&chunk->lockMutex);
    ts_fifo_queue_destroy(&chunk->blockChangeQueue);
    destroyThreadSafeMeshingState(&chunk->meshingState);
    destroyChunkMesh(chunk->renderer, chunk->chunkMesh);

    free(chunk);
    chunk = NULL;
}

Block *getBlockRefrenceInChunk(Chunk chunk, int x, int y, int z, bool generate)
{
    if (chunk == NULL)
        return NULL;

    if (x < 0 || x >= SC_LEN || z < 0 || z >= SC_LEN || y < 0 || y >= SC_LEN * SC_CNT)
        return NULL;

    if (chunk->subchunks[SC_INDEX(y)] == NULL)
        if (generate) {
            chunk->subchunks[SC_INDEX(y)] = createSubchunk();
        } else {
            return NULL;
        }

    return &chunk->subchunks[SC_INDEX(y)]->blocks[INDEX(x, y % SC_LEN, z)];
}

Block getBlockInChunk(Chunk chunk, int x, int y, int z)
{
    Block *pBlock = getBlockRefrenceInChunk(chunk, x, y, z, false);
    if (pBlock == NULL)
        return BLK_AIR;
    else
        return *pBlock;
}

void setBlockInChunk(Chunk chunk, int x, int y, int z, Block blockType)
{
    if (blockType == BLK_AIR && chunk->subchunks[y / SC_LEN]  == NULL) return;

    Block *pBlock = getBlockRefrenceInChunk(chunk, x, y, z, true);
    if (pBlock == NULL)
        return;
    else if (*pBlock != blockType)
    {
        *pBlock = blockType;
        ts_fifo_queue_enqueue(&chunk->blockChangeQueue, (void*) &(BlockChangeInfo) {.block = blockType, .x = x, .y = y, .z = z});
        setLightingState(&chunk->lightingState, LIGHT_STATE_NEEDS_LIGHTING);

        if (blockType != BLK_AIR) {
            if (y > chunk->heightMap[x + z * SC_LEN]) {
                chunk->heightMap[x + z * SC_LEN] = y;
            }

            if (x == 0) chunk->subchunks[y / SC_LEN]->border_blocks[CHUNK_NX] = true;
            if (x == SC_LEN - 1) chunk->subchunks[y / SC_LEN]->border_blocks[CHUNK_PX] = true;
            if (z == 0) chunk->subchunks[y / SC_LEN]->border_blocks[CHUNK_NZ] = true;
            if (z == SC_LEN - 1) chunk->subchunks[y / SC_LEN]->border_blocks[CHUNK_PZ] = true;

            if (y % SC_LEN == 0) {
                if (y >= SC_LEN && chunk->subchunks[(y / SC_LEN) - 1] == NULL) 
                    chunk->subchunks[(y / SC_LEN) - 1] = createSubchunk();
            } else if (y % SC_LEN == SC_LEN - 1) {
                if (y < (SC_LEN * SC_CNT - 1) && chunk->subchunks[(y / SC_LEN) + 1] == NULL) 
                    chunk->subchunks[(y / SC_LEN) + 1] = createSubchunk();
            }
        } else {
            if (y == chunk->heightMap[x + z * SC_LEN]) {
                for (uint32_t i = y; i > 0; i--) {
                    while (chunk->subchunks[i / SC_LEN] == NULL && i >= SC_LEN)
                        i -= SC_LEN;

                    if (chunk->subchunks[i / SC_LEN]->blocks[x + z * SC_LEN + (i % SC_LEN) * SC_PLN] != BLK_AIR) {
                        chunk->heightMap[x + z * SC_LEN] = i;
                        return;
                    }
                }
                chunk->heightMap[x + z * SC_LEN] = 0;
            }
        }
    }
}

Subchunk getSubchunkInChunk(Chunk chunk, uint32_t subchunkIndex, Subchunk nullReturn)
{
    if (subchunkIndex < 0 || subchunkIndex >= SC_CNT || chunk == NULL)
        return nullReturn;

    if (chunk->subchunks[subchunkIndex] == NULL)
        return nullReturn;

    return chunk->subchunks[subchunkIndex];
}

void createSubchunkInChunk(Chunk chunk, uint32_t subchunkIndex)
{
    if (subchunkIndex < 0 || subchunkIndex >= SC_CNT || chunk == NULL)
        return;

    if (chunk->subchunks[subchunkIndex] == NULL)
        chunk->subchunks[subchunkIndex] = createSubchunk();
}

// Syncronization of (Un)Locking mutexes

#define MAX_MUTEXES 10

static int compare_mutexes(const void *a, const void *b) {
    const pthread_mutex_t * const *mutex_a = a;
    const pthread_mutex_t * const *mutex_b = b;
    if (*mutex_a < *mutex_b)
        return -1;
    else if (*mutex_a > *mutex_b)
        return 1;
    else
        return 0;
}

int lock_mutexes(pthread_mutex_t *mutexes[], int count) {
    if (count <= 0)
        return 0;

    if (count > MAX_MUTEXES) {
        fprintf(stderr, "Exceeded maximum mutex count (%d)\n", MAX_MUTEXES);
        return -1;
    }

    pthread_mutex_t *sorted[MAX_MUTEXES];
    for (int i = 0; i < count; i++) {
        sorted[i] = mutexes[i];
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (sorted[i] > sorted[j]) {
                pthread_mutex_t *temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        int ret = pthread_mutex_lock(sorted[i]);
        if (ret != 0) {
            for (int j = 0; j < i; j++) {
                pthread_mutex_unlock(sorted[j]);
            }
            return ret;
        }
    }

    return 0;
}

void unlock_mutexes(pthread_mutex_t *mutexes[], int count) {
    if (count <= 0)
        return;

    if (count > MAX_MUTEXES) {
        fprintf(stderr, "Exceeded maximum mutex count (%d)\n", MAX_MUTEXES);
        return;
    }

    pthread_mutex_t *sorted[MAX_MUTEXES];
    for (int i = 0; i < count; i++) {
        sorted[i] = mutexes[i];
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (sorted[i] > sorted[j]) {
                pthread_mutex_t *temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    for (int i = count - 1; i >= 0; i--) {
        pthread_mutex_unlock(sorted[i]);
    }
}

int lock_chunks(Chunk chunks[], int count) {
    if (count <= 0)
        return 0;

    if (count > MAX_MUTEXES) {
        fprintf(stderr, "Exceeded maximum chunk count (%d)\n", MAX_MUTEXES);
        return -1;
    }

    pthread_mutex_t *mutexes[MAX_MUTEXES];
    for (int i = 0; i < count; i++) {
        mutexes[i] = &(chunks[i]->lockMutex);
    }

    return lock_mutexes(mutexes, count);
}

void unlock_chunks(Chunk chunks[], int count) {
    if (count <= 0)
        return;

    if (count > MAX_MUTEXES) {
        fprintf(stderr, "Exceeded maximum chunk count (%d)\n", MAX_MUTEXES);
        return;
    }

    pthread_mutex_t *mutexes[MAX_MUTEXES];
    for (int i = 0; i < count; i++) {
        mutexes[i] = &(chunks[i]->lockMutex);
    }

    unlock_mutexes(mutexes, count);
}

void createLightSubchunks(Chunk chunk, Chunk sidechunks[4])
{
    for (uint32_t j = 0; j < SC_CNT; j++) {
        if (chunk->subchunks[j] == NULL) {
            for (int i = 0; i < 4; i++) {
                if (sidechunks[i] && sidechunks[i]->subchunks[j] &&sidechunks[i]->subchunks[j]->border_blocks[i]) {
                    chunk->subchunks[j] = createSubchunk();
                    break;
                }
            }
        } 
        else { 
            for (int i = 0; i < 4; i++) {
                if (chunk->subchunks[j]->border_blocks[i] && sidechunks[i] && sidechunks[i]->subchunks[j] == NULL) {
                    sidechunks[i]->subchunks[j] = createSubchunk();
                }
            }
        }
    }
}
