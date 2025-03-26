@echo off

glslc.exe -O shader/skybox.vert -o shader/skybox_vert.spv
glslc.exe -O shader/skybox.frag -o shader/skybox_frag.spv
glslc.exe -O shader/block.vert -o shader/block_vert.spv
glslc.exe -O shader/block.frag -o shader/block_frag.spv
glslc.exe -O shader/tjunctionPP.vert -o shader/tjunctionPP_vert.spv
glslc.exe -O shader/tjunctionPP.frag -o shader/tjunctionPP_frag.spv
glslc.exe -O shader/debug_box.vert -o shader/debug_box_vert.spv
glslc.exe -O shader/debug_box.frag -o shader/debug_box_frag.spv


g++ -Ofast -c lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc.cpp -o lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc_win64.o

setlocal enabledelayedexpansion

REM Initialize variables
set "gcc_cmd=gcc -g -Ofast -flto -finline-functions -fprefetch-loop-arrays -Wno-aggressive-loop-optimizations -pipe lib/dvec3/dvec3.c lib/vkutils/lib/vk_mem_alloc/vk_mem_alloc_win64.o lib/vkutils/src/vkutils.c lib/datastrucs/hashmap.c lib/datastrucs/dynqueue.c lib/datastrucs/fifoqueue.c lib/datastrucs/ts_fifo.c -lstdc++"
set "output_file=cubex-engine.exe"
set "c_files="
set "additional= -lglfw3 -lm -lpthread -lvulkan-1"

REM Search for C files in the current directory
for %%i in (src\*.c) do (
    set "c_files=!c_files! %%i"
)

REM Execute gcc command with collected C files and additional linker options
if "%c_files%" == "" (
    echo No .c files found in the current directory.
) else (
    %gcc_cmd% %c_files% %additional% -o %output_file%
    if errorlevel 1 (
        echo Compilation failed.
    ) else (
        %output_file%
    )
)