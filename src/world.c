#include "world.h"
#include "mesher.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

mapNode createMapNode()
{
    mapNode_T *node = calloc(1, sizeof(mapNode_T));
    node->chunk = NULL;

    for (uint32_t i = 0; i < 4; i++)
        node->subNodes[i] = NULL;

    return node;
}

void destroyMapNode(mapNode node)
{
    if (node == NULL) return;

    for (uint32_t i = 0; i < 4; i++)
        destroyMapNode(node->subNodes[i]);

    if (node->chunk != NULL) {
        destroyChunk(node->chunk);
    }
        

    free(node);
}

Chunk getChunkRef(World world, int32_t chunkX, int32_t chunkZ, bool generateChunk)
{
    int originalX = chunkX;
    int originalZ = chunkZ;

    const int32_t half_map_size = MAP_SIZE / 2;

    if (chunkX < -half_map_size || chunkZ < -half_map_size || chunkX >= half_map_size || chunkZ >= half_map_size) {
        return NULL;
    }

    chunkX += half_map_size;
    chunkZ += half_map_size;

    mapNode current_node = world->topMapNode;
    int step = half_map_size;

    while (step > 0)
    {
        int index = (chunkX >= step) + ((chunkZ >= step) << 1);

        if (chunkX >= step) chunkX -= step;
        if (chunkZ >= step) chunkZ -= step;

        if (current_node->subNodes[index] == NULL) {
            if (!generateChunk) return NULL;
            current_node->subNodes[index] = createMapNode();
        }

        current_node = current_node->subNodes[index];
        step >>= 1;
    }

    if (current_node->chunk == NULL && generateChunk) {
        current_node->chunk = createChunk(originalX, originalZ, world->renderer);
    }

    return current_node->chunk;
}

Chunk getChunkInWorld(World world, int32_t chunkX, int32_t chunkZ)
{
    return getChunkRef(world, chunkX, chunkZ, false);
}

Chunk createChunkInWorld(World world, int32_t chunkX, int32_t chunkZ)
{
    Chunk chunk = getChunkRef(world, chunkX, chunkZ, true);
    chunk->loadedChunksNode = queue_enqueue(world->loadedChunks, chunk);
    generateTerrain(world->generator, chunk);

    Chunk sideChunk[4] = {};
    sideChunk[CHUNK_PX] = getChunkInWorld(world, chunkX + 1, chunkZ);
    sideChunk[CHUNK_NX] = getChunkInWorld(world, chunkX - 1, chunkZ);
    sideChunk[CHUNK_PZ] = getChunkInWorld(world, chunkX, chunkZ + 1);
    sideChunk[CHUNK_NZ] = getChunkInWorld(world, chunkX, chunkZ - 1);

    createLightSubchunks(chunk, sideChunk);

    return chunk;
}

void destroyChunkInWorld(World world, int32_t chunkX, int32_t chunkZ)
{
    mapNode current_node = world->topMapNode;
    const int32_t half_map_size = MAP_SIZE / 2;
    chunkX += half_map_size;
    chunkZ += half_map_size;

    int step = half_map_size;

    while (step > 0)
    {
        int index = (chunkX >= step) + ((chunkZ >= step) << 1);

        if (chunkX >= step) chunkX -= step;
        if (chunkZ >= step) chunkZ -= step;

        current_node = current_node->subNodes[index];
        if (current_node == NULL) return;
        
        step >>= 1;
    }

    if (current_node->chunk != NULL) {
        queue_remove(world->loadedChunks, current_node->chunk->loadedChunksNode);
        destroyChunk(current_node->chunk);
        current_node->chunk = NULL;
    } 
}

World createWorld(Registry registry, uint32_t renderDistance, Renderer renderer, Mesher mesher)
{
    World_T *world = calloc(1, sizeof(World_T));
    world->mesher = mesher;
    world->lightEngine = createLightEngine(registry, mesher);
    world->seed = 1000;
    world->topMapNode = createMapNode();
    world->loadedChunks = queue_create();
    world->middleX = INT32_MAX;
    world->middleZ = INT32_MAX;
    world->renderDistance = renderDistance;
    world->registry = registry;
    world->generator = createWorldGenerator(registry, world->seed);
    world->renderer = renderer;
    world->chunkDestructionQueue = fifo_queue_create(sizeof(chunkDestructionNode), 32);
    world->renderedChunks = calloc(1, sizeof(Chunk) * world->renderDistance * world->renderDistance);
    world->worldTime = TICKS_PER_DAY * 0.0;

    atomic_store(&world->created, true);
    return world;
}

void freeData(void *data) {
    return;
}

void destroyWorld(World world)
{
    fifo_queue_destroy(world->chunkDestructionQueue);
    destroyWorldGenerator(world->generator);
    queue_destroy(world->loadedChunks, freeData);
    destroyLightEngine(world->lightEngine);

    if (world->topMapNode != NULL)
        destroyMapNode(world->topMapNode);

    free(world->renderedChunks);
    free(world);
}

Chunk getChunkFromRenderedChunks(Chunk * renderedChunks, int x, int z, int renderDistance)
{
    return renderedChunks[x + z * renderDistance];
}

#include <unistd.h>

void worldTickUpdate(World world, WorldTickUpdateInfo * info)
{
    int x = (int) info->player_x;
    int z = (int) info->player_z;

    int chunkX = (x >= 0) ? x / SC_LEN : (x - (SC_LEN - 1)) / SC_LEN;
    int chunkZ = (z >= 0) ? z / SC_LEN : (z - (SC_LEN- 1)) / SC_LEN;

    bool chunkLoading = false;
 
    if (chunkX == world->middleX && chunkZ == world->middleZ) {
        chunkLoading = false;
    } else {
        world->middleX = chunkX;
        world->middleZ = chunkZ;
        chunkLoading = true;
    }

    int halfRenderDist = world->renderDistance / 2;

    Chunk tempRenderedChunks[world->renderDistance * world->renderDistance];

    if (chunkLoading) {
        memset(tempRenderedChunks, 0, sizeof(Chunk) * world->renderDistance * world->renderDistance);
    }

    QueueNode * next = world->loadedChunks->head;
    while (next != NULL) {
        QueueNode * current = next;
        next = current->next;
        Chunk chunk = (Chunk) current->data;

        int currentX = chunk->chunkX;
        int currentZ = chunk->chunkZ;

        if (currentX < (chunkX - halfRenderDist) || currentX >= (chunkX + halfRenderDist) ||
            currentZ < (chunkZ - halfRenderDist) || currentZ >= (chunkZ + halfRenderDist)) {
            atomic_store(&chunk->isRendered, false);

            if (!atomic_load(&chunk->queuedForDestruction)) {
                atomic_store(&chunk->queuedForDestruction, true);
                fifo_queue_enqueue(world->chunkDestructionQueue, (void*) &(chunkDestructionNode) {.x = currentX, .z = currentZ, .onlyVertexData = false});
            }
        } else {
            int index = (currentX - chunkX + halfRenderDist) + (currentZ - chunkZ + halfRenderDist) * world->renderDistance;
            if (index >= 0 && index < world->renderDistance * world->renderDistance) {
                tempRenderedChunks[index] = chunk;
                atomic_store(&chunk->isRendered, true);
                atomic_store(&chunk->queuedForDestruction, false);
            }
        }
    }

    if (chunkLoading)
    {
        for (int locZ = 0; locZ < world->renderDistance; locZ++) {
            for (int locX = 0; locX < world->renderDistance; locX++) {
                int index = locX + locZ * world->renderDistance;
                Chunk chunk = tempRenderedChunks[index];

                if (chunk == NULL) {
                    int newChunkX = chunkX + (locX - halfRenderDist);
                    int newChunkZ = chunkZ + (locZ - halfRenderDist);
                    chunk = createChunkInWorld(world, newChunkX, newChunkZ);
                    tempRenderedChunks[index] = chunk;
                }

                atomic_store(&chunk->isRendered, true);
                atomic_store(&chunk->queuedForDestruction, false);
            }
        }
    }

    int centerX = world->renderDistance / 2;
    int centerZ = world->renderDistance / 2;

    int offsets[CK_POS_CNT][2] = {
        {-1, 1}, {0, 1}, {1, 1},
        {-1, 0}, {0, 0}, {1, 0},
        {-1, -1}, {0, -1}, {1, -1},
    };

    for (int d = 0; d <= world->renderDistance / 2; d++) {
        for (int x = centerX - d; x <= centerX + d; x++) {
            for (int z = centerZ - d; z <= centerZ + d; z++) {
                if (abs(x - centerX) != d && abs(z - centerZ) != d) {
                    continue;
                }

                if (x < 1 || x >= world->renderDistance - 1 || z < 1 || z >= world->renderDistance - 1) {
                    continue;
                }

                int index = x + z * world->renderDistance;
                Chunk chunk = tempRenderedChunks[index];

                LightingState lightState = getLightingState(&chunk->lightingState);
                if (lightState == LIGHT_STATE_NEEDS_LIGHTING || lightState == LIGHT_STATE_UNLIT)
                {
                    lightingSubmitInfo submitInfo = {};
                    submitInfo.chunks[CK_PX] = getChunkFromRenderedChunks(tempRenderedChunks, x + 1, z, world->renderDistance);
                    submitInfo.chunks[CK_NX] = getChunkFromRenderedChunks(tempRenderedChunks, x - 1, z, world->renderDistance);
                    submitInfo.chunks[CK_PZ] = getChunkFromRenderedChunks(tempRenderedChunks, x, z + 1, world->renderDistance);
                    submitInfo.chunks[CK_NZ] = getChunkFromRenderedChunks(tempRenderedChunks, x, z - 1, world->renderDistance);
                    submitInfo.chunks[CK_PXPZ] = getChunkFromRenderedChunks(tempRenderedChunks, x + 1, z + 1, world->renderDistance);
                    submitInfo.chunks[CK_PXNZ] = getChunkFromRenderedChunks(tempRenderedChunks, x + 1, z - 1, world->renderDistance);
                    submitInfo.chunks[CK_NXPZ] = getChunkFromRenderedChunks(tempRenderedChunks, x - 1, z + 1, world->renderDistance);
                    submitInfo.chunks[CK_NXNZ] = getChunkFromRenderedChunks(tempRenderedChunks, x - 1, z - 1, world->renderDistance);
                    submitInfo.chunks[CK_ORI] = chunk;
                    submitInfo.init = (getLightingState(&chunk->lightingState) == LIGHT_STATE_UNLIT);

                    lightingEnqueue(&world->lightEngine->queue, &submitInfo);
                }

                if (getMeshingState(&chunk->meshingState) == MESH_STATE_NEEDS_MESHING) {
                    mesherSubmitInfo meshingInfo = {
                        .chunk = chunk,
                        .sidechunks[CHUNK_PX] = getChunkFromRenderedChunks(tempRenderedChunks, x + 1, z, world->renderDistance),
                        .sidechunks[CHUNK_NX] = getChunkFromRenderedChunks(tempRenderedChunks, x - 1, z, world->renderDistance),
                        .sidechunks[CHUNK_PZ] = getChunkFromRenderedChunks(tempRenderedChunks, x, z + 1, world->renderDistance),
                        .sidechunks[CHUNK_NZ] = getChunkFromRenderedChunks(tempRenderedChunks, x, z - 1, world->renderDistance),
                    };

                    mesherEnqueue(&world->mesher->queue, &meshingInfo);
                }
            }
        }
    }

    if (chunkLoading) {
        pthread_mutex_lock(&world->renderer->swapRendCksMutex);
        memcpy(world->renderedChunks, tempRenderedChunks, sizeof(Chunk) * world->renderDistance * world->renderDistance);
        pthread_mutex_unlock(&world->renderer->swapRendCksMutex);
    }

    if (atomic_load(&world->mesher->idle) && atomic_load(&world->lightEngine->idle)) {
        chunkDestructionNode node;
        while(!fifo_queue_is_empty(world->chunkDestructionQueue)) {
            fifo_queue_dequeue(world->chunkDestructionQueue, &node);

            Chunk chunk = getChunkInWorld(world, node.x, node.z);

            if (chunk == NULL)
                continue;

            if (!atomic_load(&chunk->isRendered) && atomic_load(&chunk->queuedForDestruction)) {
                destroyChunkInWorld(world, chunk->chunkX, chunk->chunkZ);
            }
        }
    }

    world->worldTime += 1;
}

void setBlockInWorld(World world, int x, int y, int z, Block block) {
    int chunkX = (x >= 0) ? x / SC_LEN : (x - (SC_LEN - 1)) / SC_LEN;
    int chunkZ = (z >= 0) ? z / SC_LEN : (z - (SC_LEN - 1)) / SC_LEN;

    int blockX = (x >= 0) ? x % SC_LEN : (SC_LEN + (x % SC_LEN)) % SC_LEN;
    int blockZ = (z >= 0) ? z % SC_LEN : (SC_LEN + (z % SC_LEN)) % SC_LEN;

    Chunk dstChunk = getChunkInWorld(world, chunkX, chunkZ);
    if (dstChunk == NULL) return;
    
    setBlockInChunk(dstChunk, blockX, y, blockZ, block);

    Chunk sideChunk[4] = {};
    if (blockX == SC_LEN - 1 && (sideChunk[CHUNK_PX] = getChunkInWorld(world, chunkX + 1, chunkZ)) != NULL) {
        setMeshingState(&sideChunk[CHUNK_PX]->meshingState, MESH_STATE_NEEDS_MESHING);
    }
        
    if (blockX == 0 && (sideChunk[CHUNK_NX] = getChunkInWorld(world, chunkX - 1, chunkZ)) != NULL) {
        setMeshingState(&sideChunk[CHUNK_NX]->meshingState, MESH_STATE_NEEDS_MESHING);
    }
        
    if (blockZ == SC_LEN - 1 && (sideChunk[CHUNK_PZ] = getChunkInWorld(world, chunkX, chunkZ + 1)) != NULL) {
        setMeshingState(&sideChunk[CHUNK_PZ]->meshingState, MESH_STATE_NEEDS_MESHING);
    }
        
    if (blockZ == 0 && (sideChunk[CHUNK_NZ] = getChunkInWorld(world, chunkX, chunkZ - 1)) != NULL) {
        setMeshingState(&sideChunk[CHUNK_NZ]->meshingState, MESH_STATE_NEEDS_MESHING);
    }

    if (y >= 0 && y < SC_CNT * SC_LEN && dstChunk->subchunks[y / SC_LEN] != NULL)
        for (int i = 0; i < 4; i++) {
            if (dstChunk->subchunks[y / SC_LEN]->border_blocks[i] &&
                sideChunk[i] &&
                sideChunk[i]->subchunks[y / SC_LEN] == NULL) {
                sideChunk[i]->subchunks[y / SC_LEN] = createSubchunk();
            }
        }
}

Block * getBlockInWorld(World world, int x, int y, int z)
{
    int chunkX = (x >= 0) ? x / SC_LEN : (x - (SC_LEN - 1)) / SC_LEN;
    int chunkZ = (z >= 0) ? z / SC_LEN : (z - (SC_LEN - 1)) / SC_LEN;

    int blockX = (x >= 0) ? x % SC_LEN : (SC_LEN + (x % SC_LEN)) % SC_LEN;
    int blockZ = (z >= 0) ? z % SC_LEN : (SC_LEN + (z % SC_LEN)) % SC_LEN;

    Chunk dstChunk = getChunkInWorld(world, chunkX, chunkZ);
    if (dstChunk == NULL) return NULL;

    return getBlockRefrenceInChunk(dstChunk, blockX, y, blockZ, false);
}

void CameraToWorldCoords(float flX, float flY, float flZ, int32_t * iX, int32_t * iY, int32_t * iZ) {
    *iX = (int32_t) floor(flX);
    *iZ = (int32_t) floor(flZ);
    *iY = (int32_t) floor(flY);
}

float getNormalizedWorldTime(World world)
{
    if (world != NULL)
        return (float) (world->worldTime % TICKS_PER_DAY) / (float) TICKS_PER_DAY;
    else {
        return 0;
    }
}