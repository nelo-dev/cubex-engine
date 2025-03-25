#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "registry.h"
#include "mesher.h"

typedef struct WorldGenerator_T {
    Registry registry;
    int seed;
} WorldGenerator_T;

typedef WorldGenerator_T * WorldGenerator;

WorldGenerator createWorldGenerator(Registry registry, int seed);
void destroyWorldGenerator(WorldGenerator generator);

#include "chunk.h"

void generateTerrain(WorldGenerator generator, Chunk chunk);

#endif