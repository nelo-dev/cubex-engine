#!/bin/bash

glslc -O shader/skybox.vert -o shader/skybox_vert.spv
glslc -O shader/skybox.frag -o shader/skybox_frag.spv
glslc -O shader/block.vert -o shader/block_vert.spv
glslc -O shader/block.frag -o shader/block_frag.spv
glslc -O shader/tjunctionPP.vert -o shader/tjunctionPP_vert.spv
glslc -O shader/tjunctionPP.frag -o shader/tjunctionPP_frag.spv
glslc -O shader/debug_box.vert -o shader/debug_box_vert.spv
glslc -O shader/debug_box.frag -o shader/debug_box_frag.spv

g++ -Ofast -c lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc.cpp -o lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc_linux64.o

# Initialize variables
gcc_cmd="gcc -Ofast -finline-functions -fprefetch-loop-arrays -Wno-aggressive-loop-optimizations -pipe lib/dvec3/dvec3.c lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc_linux64.o lib/vkutils/src/vkutils.c lib/datastrucs/hashmap.c lib/datastrucs/dynqueue.c lib/datastrucs/fifoqueue.c lib/datastrucs/ts_fifo.c -lstdc++"
output_file="cubex-engine"
c_files=""
additional="-lglfw -lm -lpthread -lvulkan"

# Search for .c files in the src directory
for file in src/*.c; do
    if [[ -f "$file" ]]; then
        c_files="$c_files $file"
    fi
done

# Check if any .c files were found
if [[ -z "$c_files" ]]; then
    echo "No .c files found in the src directory."
else
    # Compile and link
    $gcc_cmd $c_files $additional -o $output_file
    if [[ $? -ne 0 ]]; then
        echo "Compilation failed."
    else
        echo "Compilation succeeded. Running $output_file..."
        ./$output_file
    fi
fi
