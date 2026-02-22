/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>
*/
#include "../../SDL_internal.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_raspberryaudio.h"
#include "../../core/raspberry/SDL_raspberry.h"

/* 
 * BCM2835 HDMI & MAI Registers (Based on kumaashi/RaspberryPI)
 */
#define HD_BASE             0x20808000
#define HD_BUS_BASE         0x7E808000
#define HDMI_BASE           0x20902000

/* HD (MAI) 0x20808000 offsets */
#define HDMI_MAI_CTL        0x014
#define HDMI_MAI_THR        0x018
#define HDMI_MAI_FMT        0x01C
#define HDMI_MAI_DATA       0x020
#define HDMI_MAI_SMP        0x02C

/* HDMI 0x20902000 offsets */
#define HDMI_MAI_CHANNEL_MAP     0x090
#define HDMI_MAI_CONFIG          0x094
#define HDMI_MAI_FORMAT_RE       0x098
#define HDMI_AUDIO_PACKET_CONFIG 0x09C
#define HDMI_RAM_PACKET_CONFIG   0x0A0
#define HDMI_RAM_PACKET_STATUS   0x0A4
#define HDMI_CRP_CFG              0x0A8
#define HDMI_CTS_0               0x0AC
#define HDMI_CTS_1               0x0B0
#define HDMI_RAM_PACKET_START    0x400

#define BUS_ADDR_ALIAS      0x40000000 /* VideoCore alias (L2 Bypass, for DMA 하드웨어) */
#define CPU_ADDR_ALIAS      0xC0000000 /* CPU alias (L1/L2 Bypass, for CPU 직접 쓰기) */
#define DMA_PERMAP_HDMI     17

#define HD_REG(off)   (*(volatile uint32_t *)(HD_BASE + (off)))
#define HDMI_REG(off) (*(volatile uint32_t *)(HDMI_BASE + (off)))

static SDL_AudioDevice * device;
static int running;
static int locked;
static volatile int cur_buffer;

/* 4th Refinement: Quad Buffering (4 buffers) */
#define NUM_BUFFERS 4
static volatile DMA_CB dma_cb[NUM_BUFFERS] __attribute__ ((aligned (32)));
static unsigned char * dma_buffer[NUM_BUFFERS];
static Uint8 * audio_buffer;

/* 
 * 4th Refinement: Handler for Quad-buffering
 */
__attribute__((visibility("default"))) void RASPBERRYAUD_DmaInterruptHandler() {
    int16_t * src;
    
    /* Acknowledge DMA interrupt */
    DMA->ch[0].cs = DMA_INT | DMA_ACTIVE;

    /* The buffer that just finished and needs refilling */
    int fill_idx = cur_buffer;
    /* Increment and wrap around 0-3 */
    cur_buffer = (cur_buffer + 1) % NUM_BUFFERS;

    if (locked) return;

    /* Request new audio data from SDL */
    if (device->convert.needed) {
        (*device->spec.callback)(device->spec.userdata, (Uint8 *) device->convert.buf, device->convert.len);
        SDL_ConvertAudio(&device->convert);
        src = (int16_t *) device->convert.buf;
    } else {
        (*device->spec.callback)(device->spec.userdata, (Uint8 *) audio_buffer, device->spec.size);
        src = (int16_t *) audio_buffer;
    }

    /* Write samples to the uncached buffer */
    uint32_t * dst = (uint32_t *) (CPU_ADDR_ALIAS | (uint32_t)dma_buffer[fill_idx]); 
    int total_samples = device->spec.samples * device->spec.channels;
    
    for (int i = 0; i < total_samples; i++) {
        uint32_t raw = (uint32_t)(uint16_t)(*src++);
        uint32_t data = (raw << 16);
        data >>= 4; /* Preserve sign bits while shifting into audio data field */
        data &= ~0xF;
        *dst++ = data;
    }
    
    /* MAI Status Check: Recover from Underflow if occurred during high CPU load */
    if (HD_REG(HDMI_MAI_CTL) & (1 << 2)) {
        HD_REG(HDMI_MAI_CTL) |= (1 << 9); /* Flush MAI FIFO */
        HD_REG(HDMI_MAI_CTL) |= (1 << 2); /* Acknowledge UF */
    }

    __asm__ volatile ("mcr p15,0,%0,c7,c10,4" : : "r" (0)); /* DSB */
}

static void RASPBERRYAUD_CloseDevice(_THIS) {
    IRQ->irq1Disable = INTERRUPT_DMA0;
    DMA->ch[0].cs = DMA_RESET;
    HD_REG(HDMI_MAI_CTL) = 0x201; /* Reset/Flush MAI */
    if (audio_buffer) free(audio_buffer);
}

static int RASPBERRYAUD_OpenDevice(_THIS, const char *devname, int iscapture) {
    /* Enforce 44.1kHz, 2ch for asset match and stability */
    this->spec.freq = 44100;
    this->spec.channels = 2;
    this->spec.format = AUDIO_S16;
    
    /* Enforce larger buffer to reduce interrupt overhead (min 2048) */
    if (this->spec.samples < 2048) this->spec.samples = 2048;
    SDL_CalculateAudioSpec(&this->spec);

    size_t buf_size = this->spec.samples * this->spec.channels * 4;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        unsigned char * raw_ptr = (unsigned char *)malloc(buf_size + 31);
        dma_buffer[i] = (unsigned char *)(((uint32_t)raw_ptr + 31) & ~31);
        /* Clear uncached view of buffer */
        memset((void*)(CPU_ADDR_ALIAS | (uint32_t)dma_buffer[i]), 0, buf_size);
    }
    audio_buffer = (Uint8 *)malloc(this->spec.size);
    if (!audio_buffer) return SDL_SetError("Buffer fail");

    /* Initialize MAI and HDMI core */
    HD_REG(HDMI_MAI_CTL) = 0x201; 
    usleep(100);
    HD_REG(HDMI_MAI_THR) = 0x08080608;
    HD_REG(HDMI_MAI_FMT) = 0x20900;
    HD_REG(HDMI_MAI_SMP) = 0x0DCD21F3;
    HD_REG(HDMI_MAI_CTL) = (1 << 13) | (1 << 12) | (2 << 4) | (1 << 3); 

    HDMI_REG(HDMI_MAI_CONFIG) = (1 << 27) | (1 << 26) | (1 << 1) | (1 << 0);
    HDMI_REG(HDMI_MAI_CHANNEL_MAP) = 0x8;
    HDMI_REG(HDMI_AUDIO_PACKET_CONFIG) = (1 << 29) | (1 << 24) | (1 << 1) | (1 << 0);
    
    /* 9th Refinement: Dynamic Resolution Detection via Mailbox */
    uint32_t mb_addr = 0x40007000; /* Coherent memory alias for Mailbox */
    volatile uint32_t * mailbuffer = (volatile uint32_t *) mb_addr;
    uint32_t width = 1280, height = 720; /* Defaults */

    mailbuffer[0] = 8 * 4;
    mailbuffer[1] = 0;
    mailbuffer[2] = 0x00040003; /* Get Physical W/H Tag */
    mailbuffer[3] = 8;
    mailbuffer[4] = 0;
    mailbuffer[5] = 0;
    mailbuffer[6] = 0;
    mailbuffer[7] = 0;

    Raspberry_MailboxWrite(MAIL_TAGS, mb_addr);
    Raspberry_MailboxRead(MAIL_TAGS);

    if (mailbuffer[1] == 0x80000000) { /* Request Success */
        width = mailbuffer[5];
        height = mailbuffer[6];
    }

    uint32_t cts = 0x14244; /* Default to 720p-44.1k */
    if (width <= 720) {
        if (height <= 480) cts = 0x6D60;  /* 480p/VGA */
        else cts = 0x14244;              /* 720p */
    } else if (width <= 1920) {
        cts = 0x28488;                   /* 1080p */
    }

    /* ACR configuration with dynamic CTS */
    HDMI_REG(HDMI_CRP_CFG) = (1 << 24) | 6272;
    HDMI_REG(HDMI_CTS_0) = cts; 
    HDMI_REG(HDMI_CTS_1) = cts;

    /* InfoFrame configuration */
    HDMI_REG(HDMI_RAM_PACKET_CONFIG) |= (1 << 16);
    volatile uint32_t * packet4 = (volatile uint32_t *)(HDMI_BASE + HDMI_RAM_PACKET_START + (36 * 4)); 
    packet4[0] = 0x000A0184; packet4[1] = 0x00000170;
    for (int i = 2; i < 9; i++) packet4[i] = 0;

    volatile uint32_t * packet5 = (volatile uint32_t *)(HDMI_BASE + HDMI_RAM_PACKET_START + (36 * 5)); 
    packet5[0] = 0x000A0184; packet5[1] = 0x00000170;
    for (int i = 2; i < 9; i++) packet5[i] = 0;

    HDMI_REG(HDMI_RAM_PACKET_CONFIG) |= (1 << 4);
    for (int timeout = 0; timeout < 1000; timeout++) {
        if (HDMI_REG(HDMI_RAM_PACKET_STATUS) & (1 << 4)) break;
        usleep(1);
    }

    /* Configure Chained Quad-DMA with Uncached CB Access */
    for (int i = 0; i < NUM_BUFFERS; i++) {
        /* Accessing CB through Uncached Alias to ensure HW sees it immediately */
        volatile DMA_CB * cb = (volatile DMA_CB *)(CPU_ADDR_ALIAS | (uint32_t)&dma_cb[i]);
        cb->ti = DMA_SRC_INC | DMA_DEST_DREQ | (DMA_PERMAP_HDMI << 16) | (2 << 12) | DMA_INTEN;
        cb->source_ad = BUS_ADDR_ALIAS | (uint32_t)dma_buffer[i];
        cb->dest_ad = HD_BUS_BASE + HDMI_MAI_DATA; 
        cb->txfr_len = buf_size;
        cb->stride = 0;
        /* Link next buffer for zero-gap transfer */
        cb->nextconbk = BUS_ADDR_ALIAS | (uint32_t)&dma_cb[(i + 1) % NUM_BUFFERS];
    }

    device = this; cur_buffer = 0; running = 0; locked = 0;
    return 0;
}

static void RASPBERRYAUD_LockDevice(_THIS) { locked++; }
static void RASPBERRYAUD_UnlockDevice(_THIS) {
    if (this->paused) {
        IRQ->irq1Disable = INTERRUPT_DMA0; 
        DMA->ch[0].cs = DMA_RESET; 
        running = 0;
    } else if (!running) {
        IRQ->irq1Enable = INTERRUPT_DMA0; 
        DMA->enable = DMA_EN0;
        /* Start the quad-buffer chain */
        DMA->ch[0].conblk_ad = BUS_ADDR_ALIAS | (uint32_t) &dma_cb[0];
        DMA->ch[0].cs = DMA_ACTIVE; 
        cur_buffer = 0;
        running = 1;
    }
    if (locked > 0) locked--;
}

static Uint8 * RASPBERRYAUD_GetDeviceBuf(_THIS) { return audio_buffer; }
static int RASPBERRYAUD_Init(SDL_AudioDriverImpl * impl) {
    impl->OpenDevice = RASPBERRYAUD_OpenDevice;
    impl->CloseDevice = RASPBERRYAUD_CloseDevice;
    impl->LockDevice = RASPBERRYAUD_LockDevice;
    impl->UnlockDevice = RASPBERRYAUD_UnlockDevice;
    impl->GetDeviceBuf = RASPBERRYAUD_GetDeviceBuf;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->ProvidesOwnCallbackThread = 1;
    impl->SkipMixerLock = 1;
    return 1;
}

__attribute__((visibility("default"))) AudioBootStrap RASPBERRYAUD_bootstrap = { "rpi", "SDL Raspberry Pi audio driver", RASPBERRYAUD_Init, 0 };
