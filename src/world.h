#ifndef WORLD_H
#define WORLD_H

#include "worldgen.h"
#include "chunk.h"
#include "registry.h"
#include "renderer.h"
#include "../lib/datastrucs/dynqueue.h"
#include "../lib/datastrucs/fifoqueue.h"

#include <stdatomic.h>

#define MAP_SIZE        1073741824
#define TICKS_PER_DAY   24000

typedef struct mapNode_T
{
    Chunk chunk;
    struct mapNode_T * subNodes[4];
} mapNode_T;

typedef struct mapNode_T * mapNode;

typedef struct chunkDestructionNode
{
    bool onlyVertexData;
    int x, z;
} chunkDestructionNode;

typedef struct World_T
{
    int seed;
    int renderDistance;
    Registry registry;
    Renderer renderer;
    lightEngine lightEngine;
    Mesher mesher;

    uint64_t worldTime;
    int middleX, middleZ;
    mapNode topMapNode;
    Queue * loadedChunks;
    WorldGenerator generator;

    Chunk * renderedChunks;
    FifoQueue * chunkDestructionQueue;

    atomic_bool created;
} World_T;

typedef struct World_T * World;

World createWorld(Registry registry, uint32_t renderDistance, Renderer renderer, Mesher mesher);
void destroyWorld(World world);

Chunk getChunkInWorld(World world, int32_t chunkX, int32_t chunkZ);
Chunk createChunkInWorld(World world, int32_t chunkX, int32_t chunkZ);
void destroyChunkInWorld(World world, int32_t chunkX, int32_t chunkZ);

Block * getBlockInWorld(World world, int x, int y, int z);
void setBlockInWorld(World world, int x, int y, int z, Block block);
void CameraToWorldCoords(float flX, float flY, float flZ, int32_t * iX, int32_t * iY, int32_t * iZ);

typedef struct WorldTickUpdateInfo {
    double player_x;
    double player_z;
} WorldTickUpdateInfo;

void worldTickUpdate(World world, WorldTickUpdateInfo * info);
float getNormalizedWorldTime(World world);

#endif