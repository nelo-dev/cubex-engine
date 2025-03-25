#ifndef INPUT_H
#define INPUT_H

#include <pthread.h>
#include <stdbool.h>
#include "../lib/vkutils/src/vkutils.h"

typedef enum {
    KEY_NOT_PRESSED,
    KEY_PRESSED,
    KEY_RELEASED,
    KEY_HELD
} KeyState;

typedef struct {
    int key;
    KeyState state;
    int lastState;
} KeyInfo;

typedef struct InputHandler_T {
    KeyInfo *keys;
    int keyCount;
    GLFWwindow *window;

    double mouseLastX, mouseLastY;
    double mouseRelX, mouseRelY;
    double mouseSensitivity;
    bool fullscreen_toggled;

    pthread_mutex_t lock;
} InputHandler_T;

typedef InputHandler_T *InputHandler;

InputHandler createInputHandler(VkuWindow window);
void registerKey(InputHandler handler, int key);
void updateInputHandler(InputHandler handler);
KeyState getKeyState(InputHandler handler, int key);
void destroyInputHandler(InputHandler handler);

#endif
