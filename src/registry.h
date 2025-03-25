#ifndef REGISTRY_H
#define REGISTRY_H

#include "../lib/datastrucs/hashmap.h"

#define TRANSPARENT 0
#define OPAQUE      15

typedef struct {
    void *data; 
    size_t element_size;
    size_t size;
    size_t capacity;
} ExpArray;

typedef struct CustomBlockModel
{
    uint32_t vertexCount;
    float * vertices;
} CustomBlockModel;

typedef enum TexPos
{
    TEX_PX,
    TEX_NX,
    TEX_PY,
    TEX_NY,
    TEX_PZ,
    TEX_NZ
} TexPos;

typedef enum BlockModelType
{
    BLK_TYPE_NORMAL,
    BLK_TYPE_CUSTOM,
    BLK_TYPE_SLAP,
    BLK_TYPE_CROSS,
    BLK_TYPE_NONE,
    BLK_TYPE_SNOW
} BlockModelType;

typedef struct BlockInfo {
    char * name;
    char * texPaths[6];
    uint32_t texArrayIndices[6];
    uint32_t emittedLight;
    uint32_t opacity;
    BlockModelType modelType;
    CustomBlockModel * customModel;
} BlockInfo;

typedef struct InfoList_T {
    ExpArray infoArray;
    struct hashmap * hashmap;
} InfoList_T;

typedef InfoList_T * InfoList;

typedef struct Registry_T {
    InfoList blockList;
    InfoList biomeList;
} Registry_T;

typedef Registry_T * Registry;

Registry createRegistry();
void destroyRegistry(Registry registry);

uint32_t infoListGetBlockByName(InfoList list, char * name);

#endif