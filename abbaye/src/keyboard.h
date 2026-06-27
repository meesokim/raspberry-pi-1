// #include "common.h"
#include <SDL.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
typedef unsigned char byte;

typedef struct
{
    SDL_Keycode sym;
    int keyMatIdx;
    byte keyMask;
    const char *keyName; // for debugging
} KeyMap;

class CKeyboard {
    private:
        bool pressed = false;
        int repeat = 0;

        unsigned char keyMatrix[10];
        void clearMatrix() {
            for (int i = 0; i < sizeof(keyMatrix); i++)
        		keyMatrix[i] = 0xff;
            pressed = false;
            // printf("cleared\n");
        }       
        void setMatrix(const char* code);
    public:
        CKeyboard();
        void init();
        unsigned char matrix(char reg) {
            unsigned char ret = keyMatrix[(reg&0xf)];
            if (pressed && (reg&0xf) == 0)
                if (!repeat--)
                    clearMatrix();
            return ret;
        }
        void handle_event(SDL_Event);
        void ProcessSpecialKey(SDL_Keysym ksym);
        void ProcessKeyDown(SDL_Keycode sym);
        void ProcessKeyUp(SDL_Keycode sym);
        void KeyPress(char *keys);
        void KeyPress(char *key, bool, bool, bool, bool, bool);
};
