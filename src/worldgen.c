#include "worldgen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define FNL_IMPL
#include "../lib/fastnoiselite/FastNoiseLite.h"

WorldGenerator createWorldGenerator(Registry registry, int seed)
{
    WorldGenerator generator = (WorldGenerator) calloc(1, sizeof(WorldGenerator_T));
    generator->registry = registry;
    generator->seed = seed;

    return generator;
}

void destroyWorldGenerator(WorldGenerator generator)
{
    free(generator);
}

void generateTerrain(WorldGenerator generator, Chunk chunk)
{
    uint32_t waterLvl = 27;

    if (chunk == NULL)
        return;

    fnl_state fnlState = fnlCreateState();
    fnlState.seed = generator->seed;
    fnlState.noise_type = FNL_NOISE_OPENSIMPLEX2;

    uint16_t * heightmap = chunk->heightMap;
    memset(heightmap, 0, sizeof(uint16_t) * SC_PLN);

    int chunkOffsetX = chunk->chunkX * SC_LEN;
    int chunkOffsetZ = chunk->chunkZ * SC_LEN;

    int heighestBlock = 0;

    for (int32_t z = 0; z < SC_LEN; z++) {
        for (int32_t x = 0; x < SC_LEN; x++) {
            int height = 32 + (int)(fnlGetNoise2D(&fnlState, (float) (chunkOffsetX + x), (float) (chunkOffsetZ + z)) * 16.0f);
            if (height > heighestBlock)
                heighestBlock = height;
                heightmap[x + z * SC_LEN] = height;
        }
    }

    int maxHeightUsed = (heighestBlock > waterLvl) ? heighestBlock : waterLvl;
    uint32_t subchunkCount = (maxHeightUsed / SC_LEN) + 1;
    for (uint32_t i = 0; i < subchunkCount; i++) {
        if (chunk->subchunks[i] == NULL)
            chunk->subchunks[i] = createSubchunk();
    }

    for (uint32_t z = 0; z < SC_LEN; z++) {
        for (uint32_t x = 0; x < SC_LEN; x++) {
            int height = heightmap[x + z * SC_LEN];
            if (height < waterLvl - 2) {
                chunk->subchunks[SC_INDEX(height)]->blocks[BLK_INDEX(x, height % SC_LEN, z)] = 9;

                for (int y = height - 3; y < height; y++)
                if (y >= 0)
                    chunk->subchunks[SC_INDEX(y)]->blocks[BLK_INDEX(x, y % SC_LEN, z)] = 9;
            } else if (height >= waterLvl - 2 && height <= waterLvl + 2) {
                chunk->subchunks[SC_INDEX(height)]->blocks[BLK_INDEX(x, height % SC_LEN, z)] = 8;

                for (int y = height - 3; y < height; y++)
                if (y >= 0)
                    chunk->subchunks[SC_INDEX(y)]->blocks[BLK_INDEX(x, y % SC_LEN, z)] = 8;
            } else if (height > waterLvl + 2) {
                chunk->subchunks[SC_INDEX(height)]->blocks[BLK_INDEX(x, height % SC_LEN, z)] = 3;

                for (int y = height - 3; y < height; y++)
                if (y >= 0)
                    chunk->subchunks[SC_INDEX(y)]->blocks[BLK_INDEX(x, y % SC_LEN, z)] = 2;
            }

            for (int y = 0; y < height - 3; y++)
                if (y >= 0)
                    chunk->subchunks[SC_INDEX(y)]->blocks[BLK_INDEX(x, y % SC_LEN, z)] = 1;

            if (waterLvl > height) {
                for (int y = height + 1; y <= waterLvl; y++)
                    if (y >= 0)
                        chunk->subchunks[SC_INDEX(y)]->blocks[BLK_INDEX(x, y % SC_LEN, z)] = 7;
                heightmap[x + z * SC_LEN] = waterLvl;
            }
        }
    }

    if (rand() % 2 == 0)
        for (uint32_t z = 0; z < SC_LEN; z++) {
            for (uint32_t x = 0; x < SC_LEN; x++) {
                setBlockInChunk(chunk, x, 224, z, 1);
            }
        }

    for (uint32_t i = 0; i < rand() % 45; i++)
    {
        int treeX = rand() % (SC_LEN - 8) + 4;
        int treeZ = rand() % (SC_LEN - 8) + 4;
        int treeY = 32 + (int)(fnlGetNoise2D(&fnlState, (float) (chunkOffsetX + treeX), (float) (chunkOffsetZ + treeZ)) * 16.0f);
        int height = rand() % 5 + 4;

        if (treeY <= waterLvl) continue;

        for (uint32_t y = treeY + 1; y < treeY + height; y++)
            setBlockInChunk(chunk, treeX, y, treeZ, 4);

        for (uint32_t z = treeZ - 2; z < treeZ + 3; z++) {
            for (uint32_t x = treeX - 2; x < treeX + 3; x++) {
                setBlockInChunk(chunk, x, treeY + height, z, 5);
                setBlockInChunk(chunk, x, treeY + height + 1, z, 5);
            }
        }

        for (uint32_t z = treeZ - 1; z < treeZ + 2; z++) {
            for (uint32_t x = treeX - 1; x < treeX + 2; x++) {
                setBlockInChunk(chunk, x, treeY + height + 2, z, 5);
                setBlockInChunk(chunk, x, treeY + height + 3, z, 5);
            }
        }
    }

    setLightingState(&chunk->lightingState, LIGHT_STATE_NEEDS_LIGHTING);
}
