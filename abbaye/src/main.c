/* Abbaye des Morts */
/* Version 2.0 */

/* (c) 2010 - Locomalito & Gryzor87 */
/* 2013 - David "Nevat" Lara */

/* GPL v3 license */

#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"

#include "kernel.h"

typedef unsigned int uint;

extern void startscreen(SDL_Window *screen,uint *state,uint *grapset,uint *fullscreen);
extern void history(SDL_Window *screen,uint *state,uint *grapset,uint *fullscreen);
extern void game(SDL_Window *screen,uint *state,uint *grapset,uint *fullscreen);
extern void gameover (SDL_Window *screen,uint *state);
extern void ending (SDL_Window *screen,uint *state);

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef __arm__
__attribute__ ((interrupt ("IRQ"))) void interrupt_irq() {
    SDL_Interrupt_Handler();
}
#endif

#if defined(__cplusplus)
}
#endif

extern int spc1000_main(int argc, char *argv[]);

void main () {
	spc1000_main(0, NULL);
}
