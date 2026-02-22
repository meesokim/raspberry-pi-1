/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if defined(SDL_TIMER_DUMMY) || defined(SDL_TIMERS_DISABLED)

#include "SDL_timer.h"

#ifdef __RASPBERRY_PI__
// BCM2835 System Timer - free-running counter at 1 MHz (1 us resolution)
// RPi 1 (BCM2835): peripheral base = 0x20000000, system timer at +0x3000
// RPi 2/3 (BCM2836/7): peripheral base = 0x3F000000
// System timer lower 32-bit counter (CLO) at offset 0x04
#define SYSTIMER_BASE  0x20003000UL
#define SYSTIMER_CLO   (*(volatile unsigned int *)(SYSTIMER_BASE + 0x04))

static unsigned int timer_start_us = 0;
static int rpi_timer_initialized = 0;

static void rpi_timer_init(void) {
    timer_start_us = SYSTIMER_CLO;
    rpi_timer_initialized = 1;
}

// Returns milliseconds since init
static Uint32 rpi_get_ticks_ms(void) {
    if (!rpi_timer_initialized) {
        rpi_timer_init();
    }
    unsigned int now = SYSTIMER_CLO;
    // Handle 32-bit wraparound (~71 minutes): unsigned subtraction handles it
    unsigned int elapsed_us = now - timer_start_us;
    return (Uint32)(elapsed_us / 1000);
}

static void rpi_delay_ms(Uint32 ms) {
    if (!rpi_timer_initialized) {
        rpi_timer_init();
    }
    unsigned int start = SYSTIMER_CLO;
    unsigned int wait_us = ms * 1000;
    while ((SYSTIMER_CLO - start) < wait_us) {
        // busy wait
    }
}
#endif /* __RASPBERRY_PI__ */

static SDL_bool ticks_started = SDL_FALSE;

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;
#ifdef __RASPBERRY_PI__
    rpi_timer_init();
#endif
}

void
SDL_TicksQuit(void)
{
    ticks_started = SDL_FALSE;
}

Uint32
SDL_GetTicks(void)
{
    if (!ticks_started) {
        SDL_TicksInit();
    }

#ifdef __RASPBERRY_PI__
    return rpi_get_ticks_ms();
#else
    SDL_Unsupported();
    return 0;
#endif
}

Uint64
SDL_GetPerformanceCounter(void)
{
#ifdef __RASPBERRY_PI__
    return (Uint64)rpi_get_ticks_ms();
#else
    return SDL_GetTicks();
#endif
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000;
}

void
SDL_Delay(Uint32 ms)
{
#ifdef __RASPBERRY_PI__
    rpi_delay_ms(ms);
#else
    SDL_Unsupported();
#endif
}

#endif /* SDL_TIMER_DUMMY || SDL_TIMERS_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
