#include "registry.h"
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper C functions

void exp_array_init(ExpArray *array, size_t element_size, size_t initial_capacity) {
    array->data = malloc(element_size * initial_capacity);
    if (!array->data) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    array->element_size = element_size;
    array->size = 0;
    array->capacity = initial_capacity;
}

void exp_array_free(ExpArray *array) {
    free(array->data);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
}

size_t exp_array_add(ExpArray *array, const void *element) {
    if (array->size == array->capacity) {
        array->capacity *= 2;
        void *new_data = realloc(array->data, array->capacity * array->element_size);
        if (!new_data) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
        array->data = new_data;
    }
    memcpy((char *)array->data + array->size * array->element_size, element, array->element_size);
    return array->size++;
}

void *exp_array_get(ExpArray *array, size_t index) {
    if (index >= array->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return (char *)array->data + index * array->element_size;
}

void exp_array_pop(ExpArray *array) {
    if (array->size == 0) {
        fprintf(stderr, "Array is already empty\n");
        exit(EXIT_FAILURE);
    }
    array->size--;
}

// Hashmap

typedef struct hash_info {
    int id;
    char * name;
} hash_info;

int info_compare(const void *a, const void *b, void *udata) {
    const hash_info * info1 = a;
    const hash_info * info2 = b;
    return strcmp(info1->name, info2->name);
}

bool info_iter(const void * item, void * udata) {
    const hash_info * info = item;
    return true;
}

uint64_t info_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const hash_info * info = item;
    return hashmap_sip(info->name, strlen(info->name), seed0, seed1);
}

InfoList createInfoList(size_t elemSize)
{
    InfoList list = (InfoList) calloc(1, sizeof(InfoList_T));
    list->hashmap = hashmap_new(sizeof(hash_info), 0, 0, 0, info_hash, info_compare, NULL, NULL);
    exp_array_init(&list->infoArray, elemSize, 16);

    return list;
}

void destroyInfoList(InfoList infoList) {
    if (infoList == NULL)   
        return;

    exp_array_free(&infoList->infoArray);
    hashmap_free(infoList->hashmap);
    free(infoList);
}

uint32_t infoListGetBlockByName(InfoList list, char * name) {
    if (name == NULL) {
        GAME_EXIT("Error: infoListGetBlockByName(InfoList list, char * name): name is NULL");
    }
        

    const hash_info * info = hashmap_get(list->hashmap, &(hash_info) {.name = name});

    if (info == NULL) {
        return -1;
    } else {
        return info->id;
    }
}

void infoListAddBlock(InfoList blockList, BlockInfo * createInfo)
{
    if (infoListGetBlockByName(blockList, createInfo->name) != -1) {
        printf("Block \"%s\" is already registered!\n", createInfo->name);
        return;
    }
        
    uint32_t id = exp_array_add(&blockList->infoArray, (void*) createInfo);

    struct hash_info info_hash = {
        .id = id,
        .name = createInfo->name
    };

    hashmap_set(blockList->hashmap, &info_hash);
}

void registryCreateBlocks(Registry registry)
{
    char *texpaths[] = {
        "./res/tex/blocks/alpha/dirt.png",
        "./res/tex/blocks/alpha/grass_side.png",
        "./res/tex/blocks/alpha/grass_top.png",
        "./res/tex/blocks/alpha/leaves.png",
        "./res/tex/blocks/alpha/log_side.png",
        "./res/tex/blocks/alpha/log_top.png",
        "./res/tex/blocks/alpha/stone.png",
        "./res/tex/blocks/alpha/lamp.png",
        "./res/tex/blocks/alpha/water.png",
        "./res/tex/blocks/alpha/sand.png",
        "./res/tex/blocks/alpha/gravel.png"
    };

    BlockInfo block0 = {
        .name = "heavensent:air",
        .texPaths = {texpaths[6], texpaths[6], texpaths[6], texpaths[6], texpaths[6], texpaths[6]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = TRANSPARENT,
    };

    BlockInfo block1 = {
        .name = "heavensent:stone",
        .texPaths = {texpaths[6], texpaths[6], texpaths[6], texpaths[6], texpaths[6], texpaths[6]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block2 = {
        .name = "heavensent:dirt",
        .texPaths = {texpaths[0], texpaths[0], texpaths[0], texpaths[0], texpaths[0], texpaths[0]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block3 = {
        .name = "heavensent:grass",
        .texPaths = {texpaths[1], texpaths[1], texpaths[2], texpaths[0], texpaths[1], texpaths[1]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block4 = {
        .name = "heavensent:log",
        .texPaths = {texpaths[4], texpaths[4], texpaths[5], texpaths[5], texpaths[4], texpaths[4]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block5 = {
        .name = "heavensent:leaves",
        .texPaths = {texpaths[3], texpaths[3], texpaths[3], texpaths[3], texpaths[3], texpaths[3]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = 13,
    };

    BlockInfo block6 = {
        .name = "heavensent:lamp",
        .texPaths = {texpaths[7], texpaths[7], texpaths[7], texpaths[7], texpaths[7], texpaths[7]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 15,
        .opacity = OPAQUE,
    };

    BlockInfo block7 = {
        .name = "heavensent:water",
        .texPaths = {texpaths[8], texpaths[8], texpaths[8], texpaths[8], texpaths[8], texpaths[8]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block8 = {
        .name = "heavensent:sand",
        .texPaths = {texpaths[9], texpaths[9], texpaths[9], texpaths[9], texpaths[9], texpaths[9]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    BlockInfo block9 = {
        .name = "heavensent:gravel",
        .texPaths = {texpaths[10], texpaths[10], texpaths[10], texpaths[10], texpaths[10], texpaths[10]},
        .modelType = BLK_TYPE_NORMAL,
        .emittedLight = 0,
        .opacity = OPAQUE,
    };

    infoListAddBlock(registry->blockList, &block0);
    infoListAddBlock(registry->blockList, &block1);
    infoListAddBlock(registry->blockList, &block2);
    infoListAddBlock(registry->blockList, &block3);
    infoListAddBlock(registry->blockList, &block4);
    infoListAddBlock(registry->blockList, &block5);
    infoListAddBlock(registry->blockList, &block6);
    infoListAddBlock(registry->blockList, &block7);
    infoListAddBlock(registry->blockList, &block8);
    infoListAddBlock(registry->blockList, &block9);
}

Registry createRegistry()
{
    Registry registry = (Registry) calloc(1, sizeof(Registry_T));
    registry->blockList = createInfoList(sizeof(BlockInfo));

    registryCreateBlocks(registry);

    /*for (uint32_t i = 0; i < registry->blockList->infoArray.size; i++) {
        BlockInfo * info = (BlockInfo *) registry->blockList->infoArray.data;
        printf("%d = %s\n", i, info[i].name);
    }*/
        

    return registry;
}

void destroyRegistry(Registry registry)
{
    destroyInfoList(registry->blockList);
    free(registry);
}