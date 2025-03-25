#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <stdatomic.h>

typedef uint8_t Block;
typedef struct Chunk_T *Chunk;
typedef struct Subchunk_T *Subchunk;

#define SC_LEN 32
#define SC_PLN SC_LEN *SC_LEN
#define SC_VOL SC_LEN *SC_LEN *SC_LEN
#define SC_CNT 256 / SC_LEN

#define BLK_AIR 0

#include "../lib/datastrucs/ts_fifo.h"
#include "../lib/datastrucs/dynqueue.h"
#include "renderer.h"
#include "light.h"
#include "mesher.h"

#include <stdbool.h>
#include <stdatomic.h>

typedef struct BlockChangeInfo {
    int16_t x, y, z;
    Block block;
} BlockChangeInfo;

typedef struct Subchunk_T
{
    Block blocks[SC_VOL];
    LightValue light[SC_VOL];
    bool border_blocks[4];
} Subchunk_T;

typedef struct Chunk_T
{
    int32_t chunkX, chunkZ;
    Subchunk subchunks[SC_CNT];
    uint16_t heightMap[SC_PLN];
    bool lightByChunks[4];

    chunkMesh chunkMesh;
    Renderer renderer;

    TS_FIFO_Queue blockChangeQueue;
    ThreadSafeMeshingState meshingState;
    ThreadSafeLightingState lightingState;

    atomic_bool created;
    atomic_bool isRendered;
    atomic_bool queuedForDestruction;
    QueueNode * loadedChunksNode;
    pthread_mutex_t lockMutex;
} Chunk_T;

#define BLK_INDEX(x, y, z) ((x) + ((y) * SC_PLN) + ((z) * SC_LEN))
#define SC_INDEX(y) (y / SC_LEN)

Chunk createChunk(int32_t chunkX, int32_t chunkZ, Renderer renderer);
bool isChunkCreated(Chunk chunk);
void destroyChunk(Chunk chunk);
Subchunk createSubchunk();
void destroySubchunk(Subchunk subchunk);

Block *getBlockRefrenceInChunk(Chunk chunk, int x, int y, int z, bool generate);
Block getBlockInChunk(Chunk chunk, int x, int y, int z);
void setBlockInChunk(Chunk chunk, int x, int y, int z, Block blockType);
Subchunk getSubchunkInChunk(Chunk chunk, uint32_t subchunkIndex, Subchunk nullReturn);
void createSubchunkInChunk(Chunk chunk, uint32_t subchunkIndex);

void createLightSubchunks(Chunk chunk, Chunk sidechunks[4]);

int lock_chunks(Chunk chunks[], int count);
void unlock_chunks(Chunk chunks[], int count);

#endif