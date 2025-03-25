#include "mesher.h"
#include <string.h>
#include <unistd.h>

#define PACK_UINT8_TO_FLOAT(u1, u2, u3, u4_1, u4_2) ({                                           \
    uint32_t packed = ((u4_1 & 0xF) << 28) | ((u4_2 & 0xF) << 24) | (u3 << 16) | (u2 << 8) | u1; \
    *((float *)&packed);                                                                         \
})

#define PACK_UINT8_UINT16_UINT8_TO_FLOAT(u1, u2, u3) ({ \
    union                                               \
    {                                                   \
        uint8_t bytes[4];                               \
        float F;                                        \
    } packer;                                           \
    packer.bytes[0] = u1;                               \
    packer.bytes[1] = (u2) & 0xFF;                      \
    packer.bytes[2] = ((u2) >> 8) & 0xFF;               \
    packer.bytes[3] = u3;                               \
    packer.F;                                           \
})

#define PACK_UINT32_TO_FLOAT(x) ({    \
    union                             \
    {                                 \
        uint32_t u;                   \
        float f;                      \
    } uf;                             \
    _Generic((x),                     \
        uint32_t: (uf.u = (x), uf.f), \
        float: (uf.f = (x), uf.u));   \
})

#define ADD_PY_FACE(vertices, floatCount, x, y, z, xExt, zExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + 1, z);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + 1, z);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, xExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + 1, z + zExt);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, xExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + 1, z + zExt);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, xExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + 1, z + zExt);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + 1, z);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

#define ADD_NY_FACE(vertices, floatCount, x, y, z, xExt, zExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, xExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y, z + zExt);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y, z);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y, z + zExt);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, xExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z + zExt);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, xExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

#define ADD_PX_FACE(vertices, floatCount, x, y, z, zExt, yExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y, z);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y + yExt, z + zExt);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y + yExt, z);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y + yExt, z + zExt);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y, z);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + 1, y, z + zExt);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

#define ADD_NX_FACE(vertices, floatCount, x, y, z, zExt, yExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + yExt, z);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + yExt, z + zExt);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + yExt, z + zExt);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z + zExt);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(zExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

#define ADD_PZ_FACE(vertices, floatCount, x, y, z, xExt, yExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z + 1);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + yExt, z + 1);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + yExt, z + 1);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z + 1);                              \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + yExt, z + 1);                \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y, z + 1);                       \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

#define ADD_NZ_FACE(vertices, floatCount, x, y, z, xExt, yExt, texArrayIndex, sunlight, blocklight, ambient) \
    {                                                                                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + yExt, z);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y + yExt, z);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, 0, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
                                                                                                             \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x, y, z);                                  \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(xExt, yExt, ambient, sunlight, blocklight);             \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y, z);                           \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, yExt, ambient, sunlight, blocklight);                \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
        vertices[floatCount++] = PACK_UINT8_UINT16_UINT8_TO_FLOAT(x + xExt, y + yExt, z);                    \
        vertices[floatCount++] = PACK_UINT8_TO_FLOAT(0, 0, ambient, sunlight, blocklight);                   \
        vertices[floatCount++] = PACK_UINT32_TO_FLOAT(texArrayIndex);                                        \
    }

typedef struct greedyMeshSubchunkInfo {
    Mesher mesher;
    Subchunk subchunk;
    Subchunk * sideSubChunks;
    uint32_t yOffset;
    uint32_t * floatCnt;
    uint32_t bufferIndex;
} greedyMeshSubchunkInfo;

void greedyMeshSubchunk(greedyMeshSubchunkInfo * info)
{

    float * vertices = info->mesher->mappedBuffers[info->bufferIndex];
    uint32_t locFlCount = *info->floatCnt;

    uint32_t bkIndex = 0;
    BlockInfo *bkInfos = (BlockInfo *) info->mesher->registry->blockList->infoArray.data;
    uint8_t * greedyBuffer = info->mesher->greedyBuffer;

    Block *blocks = info->subchunk->blocks;
    Block *sideBlocks[6] = {
        info->sideSubChunks[SUBCHUNK_PX]->blocks,
        info->sideSubChunks[SUBCHUNK_NX]->blocks,
        info->sideSubChunks[SUBCHUNK_PY]->blocks,
        info->sideSubChunks[SUBCHUNK_NY]->blocks,
        info->sideSubChunks[SUBCHUNK_PZ]->blocks,
        info->sideSubChunks[SUBCHUNK_NZ]->blocks};

    LightValue * light = info->subchunk->light;
    LightValue *sideLights[6] = {
        info->sideSubChunks[SUBCHUNK_PX]->light,
        info->sideSubChunks[SUBCHUNK_NX]->light,
        info->sideSubChunks[SUBCHUNK_PY]->light,
        info->sideSubChunks[SUBCHUNK_NY]->light,
        info->sideSubChunks[SUBCHUNK_PZ]->light,
        info->sideSubChunks[SUBCHUNK_NZ]->light};

    memset(greedyBuffer, 0, sizeof(uint8_t) * SC_VOL);

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t k = 0;

    Mesher mesher = info->mesher;
    uint32_t bfrIndex = info->bufferIndex;

    for (uint32_t y = 0; y < SC_LEN; y++) {
        uint32_t locy = info->yOffset + y;
        for (uint32_t z = 0; z < SC_LEN; z++) {
            for (uint32_t x = 0; x < SC_LEN; x++) {
                if (blocks[bkIndex] == BLK_AIR)
                {
                    bkIndex++;
                    continue;
                }

                if (bkInfos[blocks[bkIndex]].modelType != BLK_TYPE_NORMAL)
                {
                    bkIndex++;
                    continue;                
                }

                if (mesher->bufferSizes[info->bufferIndex] < (locFlCount * sizeof(float) + NORMAL_BLOCK_REQUIRED_MEMORY))
                {
                    unmapChunkMesh(mesher->renderer, mesher->stagingbuffers[info->bufferIndex]);
                    chunkMeshResize(mesher->renderer, mesher->stagingbuffers[info->bufferIndex], mesher->bufferSizes[info->bufferIndex] * 2, VKU_BUFFER_USAGE_CPU_TO_GPU);
                    mesher->mappedBuffers[info->bufferIndex] = mapChunkMesh(mesher->renderer, mesher->stagingbuffers[info->bufferIndex]);
                    vertices = (float*) mesher->mappedBuffers[info->bufferIndex];
                    mesher->bufferSizes[info->bufferIndex] *= 2;
                }

                uint32_t *texIndices = bkInfos[blocks[bkIndex]].texArrayIndices;

                // +Y
                if (y < SC_LEN - 1) {
                    if (blocks[bkIndex + SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PY) == 0) {
                        uint32_t width = 1, height = 1;

                        while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && blocks[bkIndex + width + SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_PY) == 0 
                                && GET_COMPLETE_LIGHT(light[bkIndex + SC_PLN]) == GET_COMPLETE_LIGHT(light[bkIndex + width + SC_PLN]))
                            width++;
                                
                        while (z + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k + height * SC_LEN] != blocks[bkIndex] || blocks[bkIndex + k + height * SC_LEN + SC_PLN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_LEN], FACE_PY) == 1 
                                    || GET_COMPLETE_LIGHT(light[bkIndex + SC_PLN]) != GET_COMPLETE_LIGHT(light[bkIndex + k + height * SC_LEN + SC_PLN]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gz = 0; gz < height; gz++) {
                            for (uint32_t gx = 0; gx < width; gx++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gx + gz * SC_LEN], FACE_PY, 1);
                            }
                        }

                        ADD_PY_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PY], GET_SUNLIGHT(light[bkIndex + SC_PLN]), GET_BLOCKLIGHT(light[bkIndex + SC_PLN]), (uint8_t) LIGHT_EXP_PY);
                    }
                } else if (sideBlocks[SUBCHUNK_PY][bkIndex - SC_VOL + SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PY) == 0) {
                    uint32_t width = 1, height = 1;

                    while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && sideBlocks[SUBCHUNK_PY][bkIndex + width + SC_PLN - SC_VOL] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_PY) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PY][bkIndex + SC_PLN - SC_VOL]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PY][bkIndex + width + SC_PLN - SC_VOL]))
                            width++;

                    while (z + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k + height * SC_LEN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_PY][bkIndex + k + height * SC_LEN + SC_PLN - SC_VOL] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_LEN], FACE_PY) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PY][bkIndex + k + height * SC_LEN + SC_PLN - SC_VOL]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PY][bkIndex + SC_PLN - SC_VOL]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gz = 0; gz < height; gz++) {
                        for (uint32_t gx = 0; gx < width; gx++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gx + gz * SC_LEN], FACE_PY, 1);
                        }
                    }

                    ADD_PY_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PY], GET_SUNLIGHT(sideLights[SUBCHUNK_PY][bkIndex + SC_PLN - SC_VOL]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_PY][bkIndex + SC_PLN - SC_VOL]), (uint8_t) LIGHT_EXP_PY);
                }

                // -Y
                if (y > 0) {
                    if (blocks[bkIndex - SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NY) == 0) {
                        uint32_t width = 1, height = 1;

                        while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && blocks[bkIndex + width - SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_NY) == 0
                                && GET_COMPLETE_LIGHT(light[bkIndex - SC_PLN]) == GET_COMPLETE_LIGHT(light[bkIndex + width - SC_PLN]))
                            width++;
                                
                        while (z + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k + height * SC_LEN] != blocks[bkIndex] || blocks[bkIndex + k + height * SC_LEN - SC_PLN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_LEN], FACE_NY) == 1
                                    || GET_COMPLETE_LIGHT(light[bkIndex - SC_PLN]) != GET_COMPLETE_LIGHT(light[bkIndex + k + height * SC_LEN - SC_PLN]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gz = 0; gz < height; gz++) {
                            for (uint32_t gx = 0; gx < width; gx++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gx + gz * SC_LEN], FACE_NY, 1);
                            }
                        }

                        ADD_NY_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NY], GET_SUNLIGHT(light[bkIndex - SC_PLN]), GET_BLOCKLIGHT(light[bkIndex - SC_PLN]), (uint8_t) LIGHT_EXP_NY);
                    }
                } else if (sideBlocks[SUBCHUNK_NY][bkIndex + SC_VOL - SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NY) == 0) {
                    uint32_t width = 1, height = 1;

                    while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && sideBlocks[SUBCHUNK_NY][bkIndex + width - SC_PLN + SC_VOL] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_NY) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NY][bkIndex - SC_PLN + SC_VOL]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NY][bkIndex + width - SC_PLN + SC_VOL]))
                            width++;

                    while (z + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k + height * SC_LEN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_NY][bkIndex + k + height * SC_LEN - SC_PLN + SC_VOL] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_LEN], FACE_NY) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NY][bkIndex + k + height * SC_LEN - SC_PLN + SC_VOL]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NY][bkIndex - SC_PLN + SC_VOL]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gz = 0; gz < height; gz++) {
                        for (uint32_t gx = 0; gx < width; gx++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gx + gz * SC_LEN], FACE_NY, 1);
                        }
                    }

                    ADD_NY_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NY], GET_SUNLIGHT(sideLights[SUBCHUNK_NY][bkIndex - SC_PLN + SC_VOL]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_NY][bkIndex - SC_PLN + SC_VOL]), (uint8_t) LIGHT_EXP_NY);
                }

                // +X
                if (x < SC_LEN - 1) {
                    if (blocks[bkIndex + 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PX) == 0) {
                        uint32_t width = 1, height = 1;

                        while (z + width < SC_LEN && blocks[bkIndex + width * SC_LEN] == blocks[bkIndex] && blocks[bkIndex + width * SC_LEN + 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width * SC_LEN], FACE_PX) == 0
                                && GET_COMPLETE_LIGHT(light[bkIndex + 1]) == GET_COMPLETE_LIGHT(light[bkIndex + 1 + width * SC_LEN]))
                            width++;
                                
                        while (y + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k * SC_LEN + height * SC_PLN] != blocks[bkIndex] || blocks[bkIndex + k * SC_LEN + height * SC_PLN + 1] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k * SC_LEN + height * SC_PLN], FACE_PX) == 1
                                    || GET_COMPLETE_LIGHT(light[bkIndex + k * SC_LEN + height * SC_PLN + 1]) != GET_COMPLETE_LIGHT(light[bkIndex + 1]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gy = 0; gy < height; gy++) {
                            for (uint32_t gz = 0; gz < width; gz++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gz * SC_LEN], FACE_PX, 1);
                            }
                        }

                        ADD_PX_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PX], GET_SUNLIGHT(light[bkIndex + 1]), GET_BLOCKLIGHT(light[bkIndex + 1]), (uint8_t) LIGHT_EXP_PX);
                    }
                } else if (sideBlocks[SUBCHUNK_PX][bkIndex - SC_LEN + 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PX) == 0) {
                    uint32_t width = 1, height = 1;

                    while (z + width < SC_LEN && blocks[bkIndex + width * SC_LEN] == blocks[bkIndex] && sideBlocks[SUBCHUNK_PX][bkIndex + width * SC_LEN + 1 - SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width * SC_LEN], FACE_PX) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PX][bkIndex + 1 - SC_LEN]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PX][bkIndex + width * SC_LEN - SC_LEN + 1]))
                        width++;
                            
                    while (y + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k * SC_LEN + height * SC_PLN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_PX][bkIndex + k * SC_LEN + height * SC_PLN + 1 - SC_LEN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k * SC_LEN + height * SC_PLN], FACE_PX) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PX][bkIndex + 1 - SC_LEN]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PX][bkIndex + k * SC_LEN + height * SC_PLN - SC_LEN + 1]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gy = 0; gy < height; gy++) {
                        for (uint32_t gz = 0; gz < width; gz++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gz * SC_LEN], FACE_PX, 1);
                        }
                    }

                    ADD_PX_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PX], GET_SUNLIGHT(sideLights[SUBCHUNK_PX][bkIndex + 1 - SC_LEN]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_PX][bkIndex + 1 - SC_LEN]), (uint8_t) LIGHT_EXP_PX);
                }

                //-X
                if (x > 0) {
                    if (blocks[bkIndex - 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NX) == 0) {
                        uint32_t width = 1, height = 1;

                        while (z + width < SC_LEN && blocks[bkIndex + width * SC_LEN] == blocks[bkIndex] && blocks[bkIndex + width * SC_LEN - 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width * SC_LEN], FACE_NX) == 0
                                && GET_COMPLETE_LIGHT(light[bkIndex - 1]) == GET_COMPLETE_LIGHT(light[bkIndex - 1 + width * SC_LEN]))
                            width++;
                                
                        while (y + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k * SC_LEN + height * SC_PLN] != blocks[bkIndex] || blocks[bkIndex + k * SC_LEN + height * SC_PLN - 1] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k * SC_LEN + height * SC_PLN], FACE_NX) == 1
                                    || GET_COMPLETE_LIGHT(light[bkIndex + k * SC_LEN + height * SC_PLN - 1]) != GET_COMPLETE_LIGHT(light[bkIndex - 1]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gy = 0; gy < height; gy++) {
                            for (uint32_t gz = 0; gz < width; gz++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gz * SC_LEN], FACE_NX, 1);
                            }
                        }

                        ADD_NX_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NX], GET_SUNLIGHT(light[bkIndex - 1]), GET_BLOCKLIGHT(light[bkIndex - 1]), (uint8_t) LIGHT_EXP_NX);
                    }
                } else if (sideBlocks[SUBCHUNK_NX][bkIndex + SC_LEN - 1] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NX) == 0) {
                    uint32_t width = 1, height = 1;

                    while (z + width < SC_LEN && blocks[bkIndex + width * SC_LEN] == blocks[bkIndex] && sideBlocks[SUBCHUNK_NX][bkIndex + width * SC_LEN - 1 + SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width * SC_LEN], FACE_NX) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NX][bkIndex - 1 + SC_LEN]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NX][bkIndex + width * SC_LEN + SC_LEN - 1]))
                        width++;
                            
                    while (y + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k * SC_LEN + height * SC_PLN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_NX][bkIndex + k * SC_LEN + height * SC_PLN - 1 + SC_LEN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k * SC_LEN + height * SC_PLN], FACE_NX) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NX][bkIndex - 1 + SC_LEN]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NX][bkIndex + k * SC_LEN + height * SC_PLN + SC_LEN - 1]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gy = 0; gy < height; gy++) {
                        for (uint32_t gz = 0; gz < width; gz++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gz * SC_LEN], FACE_NX, 1);
                        }
                    }

                    ADD_NX_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NX], GET_SUNLIGHT(sideLights[SUBCHUNK_NX][bkIndex - 1 + SC_LEN]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_NX][bkIndex - 1 + SC_LEN]), (uint8_t) LIGHT_EXP_NX);
                }

                // +Z
                if (z < SC_LEN - 1) {
                    if (blocks[bkIndex + SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PZ) == 0) {
                        uint32_t width = 1, height = 1;

                        while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && blocks[bkIndex + width + SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_PZ) == 0
                                && GET_COMPLETE_LIGHT(light[bkIndex + width + SC_LEN]) == GET_COMPLETE_LIGHT(light[bkIndex + SC_LEN]))
                            width++;
                                
                        while (y + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k + height * SC_PLN] != blocks[bkIndex] || blocks[bkIndex + k + height * SC_PLN + SC_LEN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_PLN], FACE_PZ) == 1
                                    || GET_COMPLETE_LIGHT(light[bkIndex + k + height * SC_PLN + SC_LEN]) != GET_COMPLETE_LIGHT(light[bkIndex + SC_LEN]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gy = 0; gy < height; gy++) {
                            for (uint32_t gx = 0; gx < width; gx++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gx], FACE_PZ, 1);
                            }
                        }

                        ADD_PZ_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PZ], GET_SUNLIGHT(light[bkIndex + SC_LEN]), GET_BLOCKLIGHT(light[bkIndex + SC_LEN]), (uint8_t) LIGHT_EXP_PZ);
                    }
                } else if (sideBlocks[SUBCHUNK_PZ][bkIndex - SC_PLN + SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_PZ) == 0) {
                    uint32_t width = 1, height = 1;

                    while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && sideBlocks[SUBCHUNK_PZ][bkIndex + width + SC_LEN - SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_PZ) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PZ][bkIndex + width + SC_LEN - SC_PLN]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PZ][bkIndex + SC_LEN - SC_PLN]))
                        width++;
                            
                    while (y + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k + height * SC_PLN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_PZ][bkIndex + k + height * SC_PLN + SC_LEN - SC_PLN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_PLN], FACE_PZ) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PZ][bkIndex + k + height * SC_PLN + SC_LEN - SC_PLN]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_PZ][bkIndex + SC_LEN - SC_PLN]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gy = 0; gy < height; gy++) {
                        for (uint32_t gx = 0; gx < width; gx++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gx], FACE_PZ, 1);
                        }
                    }

                    ADD_PZ_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_PZ], GET_SUNLIGHT(sideLights[SUBCHUNK_PZ][bkIndex + SC_LEN - SC_PLN]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_PZ][bkIndex + SC_LEN - SC_PLN]), (uint8_t) LIGHT_EXP_PZ);
                }

                // -Z
                if (z > 0) {
                    if (blocks[bkIndex - SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NZ) == 0) {
                        uint32_t width = 1, height = 1;

                        while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && blocks[bkIndex + width - SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_NZ) == 0
                                && GET_COMPLETE_LIGHT(light[bkIndex + width - SC_LEN]) == GET_COMPLETE_LIGHT(light[bkIndex - SC_LEN]))
                            width++;
                                
                        while (y + height < SC_LEN) {
                            uint32_t k = 0;
                            for (k = 0; k < width; k++) {
                                if (blocks[bkIndex + k + height * SC_PLN] != blocks[bkIndex] || blocks[bkIndex + k + height * SC_PLN - SC_LEN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_PLN], FACE_NZ) == 1
                                    || GET_COMPLETE_LIGHT(light[bkIndex + k + height * SC_PLN - SC_LEN]) != GET_COMPLETE_LIGHT(light[bkIndex - SC_LEN]))
                                    break;
                            }

                            if (k == width) height++;
                            else break;
                        }

                        for (uint32_t gy = 0; gy < height; gy++) {
                            for (uint32_t gx = 0; gx < width; gx++) {
                                SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gx], FACE_NZ, 1);
                            }
                        }

                        ADD_NZ_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NZ], GET_SUNLIGHT(light[bkIndex - SC_LEN]), GET_BLOCKLIGHT(light[bkIndex - SC_LEN]), (uint8_t) LIGHT_EXP_NZ);
                    }
                } else if (sideBlocks[SUBCHUNK_NZ][bkIndex + SC_PLN - SC_LEN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex], FACE_NZ) == 0) {
                    uint32_t width = 1, height = 1;

                    while (x + width < SC_LEN && blocks[bkIndex + width] == blocks[bkIndex] && sideBlocks[SUBCHUNK_NZ][bkIndex + width - SC_LEN + SC_PLN] == BLK_AIR && GET_BIT(greedyBuffer[bkIndex + width], FACE_NZ) == 0
                            && GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NZ][bkIndex + width - SC_LEN + SC_PLN]) == GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NZ][bkIndex - SC_LEN + SC_PLN]))
                        width++;
                            
                    while (y + height < SC_LEN) {
                        uint32_t k = 0;
                        for (k = 0; k < width; k++) {
                            if (blocks[bkIndex + k + height * SC_PLN] != blocks[bkIndex] || sideBlocks[SUBCHUNK_NZ][bkIndex + k + height * SC_PLN - SC_LEN + SC_PLN] != BLK_AIR || GET_BIT(greedyBuffer[bkIndex + k + height * SC_PLN], FACE_NZ) == 1
                                || GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NZ][bkIndex + k + height * SC_PLN - SC_LEN + SC_PLN]) != GET_COMPLETE_LIGHT(sideLights[SUBCHUNK_NZ][bkIndex - SC_LEN + SC_PLN]))
                                break;
                        }

                        if (k == width) height++;
                        else break;
                    }

                    for (uint32_t gy = 0; gy < height; gy++) {
                        for (uint32_t gx = 0; gx < width; gx++) {
                            SET_BIT_VALUE(greedyBuffer[bkIndex + gy * SC_PLN + gx], FACE_NZ, 1);
                        }
                    }

                    ADD_NZ_FACE(vertices, locFlCount, (uint8_t)x, (uint16_t)locy, (uint8_t)z, width, height, texIndices[TEX_NZ], GET_SUNLIGHT(sideLights[SUBCHUNK_NZ][bkIndex - SC_LEN + SC_PLN]), GET_BLOCKLIGHT(sideLights[SUBCHUNK_NZ][bkIndex - SC_LEN + SC_PLN]), (uint8_t) LIGHT_EXP_NZ);
                }

                bkIndex++;
            }
        }
    }

    *info->floatCnt = locFlCount;
}

typedef struct greedyMeshChunkInfo {
    Mesher mesher; 
    Chunk chunk; 
    Chunk * sideChunks;
    uint32_t bufferIndex;
    uint32_t * floatCnt;
} greedyMeshChunkInfo;

void greedyMeshChunk(greedyMeshChunkInfo * info)
{
    for (uint32_t scIndex = 0; scIndex < SC_CNT; scIndex++)
    {
        if (info->chunk->subchunks[scIndex] == NULL)
            continue;

        Subchunk sideSubchunks[6];
        sideSubchunks[SUBCHUNK_PX] = getSubchunkInChunk(info->sideChunks[CHUNK_PX], scIndex, info->mesher->airSubchunk);
        sideSubchunks[SUBCHUNK_NX] = getSubchunkInChunk(info->sideChunks[CHUNK_NX], scIndex, info->mesher->airSubchunk);
        sideSubchunks[SUBCHUNK_PZ] = getSubchunkInChunk(info->sideChunks[CHUNK_PZ], scIndex, info->mesher->airSubchunk);
        sideSubchunks[SUBCHUNK_NZ] = getSubchunkInChunk(info->sideChunks[CHUNK_NZ], scIndex, info->mesher->airSubchunk);
        sideSubchunks[SUBCHUNK_PY] = getSubchunkInChunk(info->chunk, scIndex + 1, info->mesher->airSubchunk);
        sideSubchunks[SUBCHUNK_NY] = getSubchunkInChunk(info->chunk, scIndex - 1, info->mesher->airSubchunk);

        greedyMeshSubchunkInfo scInfo = {
            .floatCnt = info->floatCnt,
            .mesher = info->mesher,
            .sideSubChunks = sideSubchunks,
            .subchunk = info->chunk->subchunks[scIndex],
            .yOffset = scIndex * SC_LEN,
            .bufferIndex = info->bufferIndex,
        };

        greedyMeshSubchunk(&scInfo);
    }
}

// Mesher

void createMesherQueue(TS_FIFO_Queue * meshQueue) {
    ts_fifo_queue_init(meshQueue, sizeof(mesherSubmitInfo), INITIAL_MESH_QUEUE_CAP);
}

void mesherEnqueue(TS_FIFO_Queue * meshQueue, mesherSubmitInfo *info) {
    setMeshingState(&info->chunk->meshingState, MESH_STATE_QUEUED);
    ts_fifo_queue_enqueue(meshQueue, info);
}

int mesherDequeue(TS_FIFO_Queue * meshQueue, mesherSubmitInfo *info) {
    if (!info) return -1;

    if (ts_fifo_queue_dequeue(meshQueue, info) != 0) {
        return -1;
    }

    if (info->stopState == 1) {
        return 1;
    }

    if (getMeshingState(&info->chunk->meshingState) != MESH_STATE_QUEUED) {
        return -1;
    }

    return 0;
}

void destroyMesherQueue(TS_FIFO_Queue * meshQueue) {
    if (meshQueue) {
        ts_fifo_queue_destroy(meshQueue);
    }
}

#include <unistd.h>

void *mesherFunction(void *arg)
{
    Mesher mesher = (Mesher)arg;

    uint32_t activeBuffer = 0;
    uint32_t floatCnts[BATCHED_UPLOAD_BUFFER_CNT];
    chunkMesh lastMeshes[BATCHED_UPLOAD_BUFFER_CNT];
    Chunk lastChunks[BATCHED_UPLOAD_BUFFER_CNT];

    for (uint32_t i = 0; i < BATCHED_UPLOAD_BUFFER_CNT; i++)
    {
        mesher->bufferSizes[i] = INITIAL_MESHING_VERTEX_BUFFER_SIZE;
        mesher->stagingbuffers[i] = createChunkMesh(mesher->renderer);
        chunkMeshInit(mesher->renderer, mesher->stagingbuffers[i], INITIAL_MESHING_VERTEX_BUFFER_SIZE, VKU_BUFFER_USAGE_CPU_TO_GPU);
        mesher->mappedBuffers[i] = mapChunkMesh(mesher->renderer, mesher->stagingbuffers[i]);

        floatCnts[i] = 0;
        lastMeshes[i] = NULL;
    }

    while (1)
    {
        if (ts_fifo_queue_count(&mesher->queue) == 0)
            atomic_store(&mesher->idle, true);

        mesherSubmitInfo info;
        int res = mesherDequeue(&mesher->queue, &info);
        atomic_store(&mesher->idle, false);

        if (res == -1) {
            continue;
        } else if (res == 1) {
            for (uint32_t i = 0; i < BATCHED_UPLOAD_BUFFER_CNT; i++)
            {
                unmapChunkMesh(mesher->renderer, mesher->stagingbuffers[i]);
                destroyChunkMesh(mesher->renderer, mesher->stagingbuffers[i]);
            }
            break;
        }

        if (!atomic_load(&info.chunk->isRendered))
            continue;

        Chunk lockingChunks[5] = {
            info.chunk,
            info.sidechunks[0],
            info.sidechunks[1],
            info.sidechunks[2],
            info.sidechunks[3],
        };

        lock_chunks(lockingChunks, 5);

        setMeshingState(&info.chunk->meshingState, MESH_STATE_IN_PROGRESS);

        greedyMeshChunkInfo meshInfo = {
            .chunk = info.chunk,
            .sideChunks = info.sidechunks,
            .mesher = mesher,
            .bufferIndex = activeBuffer,
            .floatCnt = &(floatCnts[activeBuffer]),
        };

        greedyMeshChunk(&meshInfo);
        
        if (floatCnts[activeBuffer] <= 0)
        {
            unlock_chunks(lockingChunks, 5);
            continue;
        }

        lastMeshes[activeBuffer] = info.chunk->chunkMesh;
        lastChunks[activeBuffer] = info.chunk;
        activeBuffer++;

        unlock_chunks(lockingChunks, 5);

        if (activeBuffer >= BATCHED_UPLOAD_BUFFER_CNT || (ts_fifo_queue_count(&mesher->queue) == 0 && activeBuffer > 0))
        {
            VkDeviceSize bufferSizes[BATCHED_UPLOAD_BUFFER_CNT] = {0};

            for (uint32_t i = 0; i < activeBuffer; i++) {
                atomic_store(&lastMeshes[i]->swap_cond, true);
            }
            
            for (uint32_t i = 0; i < activeBuffer; i++) {
                chunkMeshInit(mesher->renderer, lastMeshes[i], floatCnts[i] * sizeof(float), VKU_BUFFER_USAGE_GPU_ONLY); // 1st thread
                bufferSizes[i] = floatCnts[i] * sizeof(float);
                floatCnts[i] = 0;
            }

            chunkMeshCopy(mesher->renderer, mesher->stagingbuffers, lastMeshes, bufferSizes, activeBuffer);
            
            for (uint32_t i = 0; i < activeBuffer; i++) {
                atomic_store(&lastMeshes[i]->swap_cond, false);
                setMeshingState(&lastChunks[i]->meshingState, MESH_STATE_MESHED);
            }

            activeBuffer = 0;
        }
    }

    return NULL;
}

Mesher createMesher(Registry registry, Renderer renderer)
{
    Mesher_T *mesher = calloc(1, sizeof(Mesher_T));
    if (!mesher)
        return NULL;

    mesher->renderer = renderer;
    mesher->registry = registry;
    createMesherQueue(&mesher->queue);
    mesher->airSubchunk = createSubchunk();

    memset(mesher->airSubchunk->blocks, BLK_AIR, sizeof(Block) * SC_VOL);

    for (uint32_t i = 0; i < SC_VOL; i++) {
        SET_SUNLIGHT(mesher->airSubchunk->light[i], UINT4_MAX);
        SET_BLOCKLIGHT(mesher->airSubchunk->light[i], 0);
    }

    if (pthread_create(&mesher->thread, NULL, mesherFunction, mesher) != 0)
    {
        destroyMesher(mesher);
        return NULL;
    }

    return mesher;
}

void destroyMesher(Mesher mesher)
{
    if (!mesher)
        return;

    mesherSubmitInfo sentinel = {.chunk = NULL, .sidechunks = NULL, .stopState = 1};
    ts_fifo_queue_enqueue(&mesher->queue, &sentinel);

    pthread_join(mesher->thread, NULL);     

    destroyMesherQueue(&mesher->queue);
    destroySubchunk(mesher->airSubchunk);
    free(mesher);
}

// ThreadSafeMeshState

void initThreadSafeMeshingState(ThreadSafeMeshingState *meshingState, MeshingState initialState) {
    meshingState->state = initialState;
    pthread_mutex_init(&meshingState->mutex, NULL);
}

void setMeshingState(ThreadSafeMeshingState *meshingState, MeshingState newState) {
    pthread_mutex_lock(&meshingState->mutex);

    if (meshingState->state == MESH_STATE_QUEUED && newState == MESH_STATE_NEEDS_MESHING)
    {
        pthread_mutex_unlock(&meshingState->mutex);
        return;
    }

    meshingState->state = newState;
    pthread_mutex_unlock(&meshingState->mutex);
}

MeshingState getMeshingState(ThreadSafeMeshingState *meshingState) {
    MeshingState currentState;
    pthread_mutex_lock(&meshingState->mutex);
    currentState = meshingState->state;
    pthread_mutex_unlock(&meshingState->mutex);
    return currentState;
}

void destroyThreadSafeMeshingState(ThreadSafeMeshingState *meshingState) {
    pthread_mutex_destroy(&meshingState->mutex);
}

