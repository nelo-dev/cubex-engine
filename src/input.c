#include "input.h"
#include <stdlib.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "game.h"

InputHandler createInputHandler(VkuWindow window) {
    InputHandler_T *handler = (InputHandler)malloc(sizeof(InputHandler_T));
    handler->keys = NULL;
    handler->keyCount = 0;
    handler->window = window->glfwWindow;

    pthread_mutex_init(&handler->lock, NULL); // Initialize the mutex

    registerKey(handler, GLFW_KEY_F11);
    registerKey(handler, GLFW_KEY_ESCAPE);
    registerKey(handler, GLFW_KEY_W);
    registerKey(handler, GLFW_KEY_S);
    registerKey(handler, GLFW_KEY_A);
    registerKey(handler, GLFW_KEY_D);
    registerKey(handler, GLFW_KEY_P);
    registerKey(handler, GLFW_KEY_R);
    registerKey(handler, GLFW_KEY_LEFT_CONTROL);
    registerKey(handler, GLFW_KEY_LEFT_SHIFT);
    registerKey(handler, GLFW_KEY_SPACE);
    registerKey(handler, GLFW_KEY_F12);
    registerKey(handler, GLFW_KEY_M);
    registerKey(handler, GLFW_KEY_G);
    registerKey(handler, GLFW_KEY_F7);

    handler->mouseLastX = 0.0f;
    handler->mouseLastY = 0.0f;
    handler->mouseSensitivity = 0.1f;
    handler->fullscreen_toggled = false;

    return handler;
}

void registerKey(InputHandler handler, int key) {
    pthread_mutex_lock(&handler->lock); // Lock mutex
    handler->keys = (KeyInfo *)realloc(handler->keys, (handler->keyCount + 1) * sizeof(KeyInfo));
    handler->keys[handler->keyCount].key = key;
    handler->keys[handler->keyCount].state = KEY_NOT_PRESSED;
    handler->keys[handler->keyCount].lastState = GLFW_RELEASE;
    handler->keyCount++;
    pthread_mutex_unlock(&handler->lock); // Unlock mutex
}

void updateInputHandler(InputHandler handler) {
    pthread_mutex_lock(&handler->lock); // Lock mutex
    for (int i = 0; i < handler->keyCount; i++) {
        int currentState = glfwGetKey(handler->window, handler->keys[i].key);
        if (currentState == GLFW_PRESS) {
            if (handler->keys[i].lastState == GLFW_RELEASE) {
                handler->keys[i].state = KEY_PRESSED;
            } else {
                handler->keys[i].state = KEY_HELD;  // Key held down
            }
        } else if (currentState == GLFW_RELEASE) {
            if (handler->keys[i].lastState == GLFW_PRESS) {
                handler->keys[i].state = KEY_RELEASED;
            } else {
                handler->keys[i].state = KEY_NOT_PRESSED;
            }
        }
        handler->keys[i].lastState = currentState;
    }

    if (!handler->fullscreen_toggled) {
        double cursorX, cursorY;
        glfwGetCursorPos(handler->window, &cursorX, &cursorY);

        if (handler->mouseLastX == 0.0 && handler->mouseLastY == 0.0) {
            handler->mouseLastX = cursorX;
            handler->mouseLastY = cursorY;
        }

        handler->mouseRelX = cursorX - handler->mouseLastX;
        handler->mouseRelY = cursorY - handler->mouseLastY;

        handler->mouseLastX = cursorX;
        handler->mouseLastY = cursorY;
    } else {
        handler->mouseRelX = 0;
        handler->mouseRelY = 0;

        double cursorX, cursorY;
        glfwGetCursorPos(handler->window, &cursorX, &cursorY);
        handler->mouseLastX = cursorX;
        handler->mouseLastY = cursorY;

        handler->fullscreen_toggled = false;
    }
    pthread_mutex_unlock(&handler->lock); // Unlock mutex
}

KeyState getKeyState(InputHandler handler, int key) {
    pthread_mutex_lock(&handler->lock); // Lock mutex
    KeyState state = KEY_NOT_PRESSED;
    for (int i = 0; i < handler->keyCount; i++) {
        if (handler->keys[i].key == key) {
            state = handler->keys[i].state;
            break;
        }
    }
    pthread_mutex_unlock(&handler->lock); // Unlock mutex
    return state;
}

void destroyInputHandler(InputHandler handler) {
    pthread_mutex_lock(&handler->lock); // Lock mutex
    free(handler->keys);
    pthread_mutex_unlock(&handler->lock); // Unlock mutex

    pthread_mutex_destroy(&handler->lock); // Destroy the mutex
    free(handler);
}