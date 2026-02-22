/*
 * Copyright (c) 2014 Marco Maccaferri and Others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include "audio.h"

#define SAMPLE_SHIFT       3

/*
 * BCM2835 HDMI Multi-channel Audio Interconnect (MAI) registers
 * ARM physical base: PERIPHERAL_BASE + 0x902000 = 0x20902000
 * Bus address:       0x7E902000
 *
 * Register offsets (from Linux kernel drivers/gpu/drm/vc4/vc4_hdmi.c):
 */
#define HDMI_PHYS_BASE      (PERIPHERAL_BASE + 0x902000)
#define HDMI_BUS_BASE       0x7E902000

/* MAI (Multi-channel Audio Interconnect) register offsets */
#define HDMI_MAI_CTL_OFFSET     0x014   /* MAI Control */
#define HDMI_MAI_FMT_OFFSET     0x018   /* MAI Sample Format */
#define HDMI_MAI_DATA_OFFSET    0x01C   /* MAI FIFO Data */
#define HDMI_MAI_SMP_OFFSET     0x020   /* MAI Sample Rate */
#define HDMI_MAI_THR_OFFSET     0x024   /* MAI FIFO Threshold */

/* MAI CTL bits */
#define HDMI_MAI_CTL_EN         (1 << 0)    /* MAI enable */
#define HDMI_MAI_CTL_WHOLSMP    (1 << 2)    /* Whole sample enable */
#define HDMI_MAI_CTL_CHNUM(n)   ((n) << 4) /* Number of channels */

/* MAI Format bits */
#define HDMI_MAI_FORMAT_16BIT   (0 << 4)    /* 16-bit samples */
#define HDMI_MAI_FORMAT_20BIT   (1 << 4)
#define HDMI_MAI_FORMAT_24BIT   (2 << 4)

/* DMA Peripheral Mapping for HDMI MAI - use unpaced (no DREQ needed) */
#define HDMI_DMA_TI  (DMA_SRC_INC | DMA_INTEN)

/* Volatile pointer to MAI registers */
#define MAI_REG(offset)  (*(volatile uint32_t *)(HDMI_PHYS_BASE + (offset)))

static uint32_t            max_samples;

static volatile int        cur_buffer;
static volatile DMA_CB     dma_cb[2];
static unsigned char     * dma_buffer[2];

static volatile uint32_t * write_ptr;
static volatile uint32_t   write_size;

static int16_t           * audio_buffer;

int audio_open(uint32_t samples) {
    max_samples = samples * 2;   /* stereo: left + right */

    for (int i = 0; i < 2; i++) {
        if (dma_buffer[i] != NULL) {
            free(dma_buffer[i]);
        }
        /* Extra 15 bytes for 16-byte alignment */
        dma_buffer[i] = (unsigned char *)malloc(max_samples * 4 + 15);
        if (dma_buffer[i] == NULL) {
            return -1;
        }
        memset(dma_buffer[i], 0, max_samples * 4 + 15);
    }

    audio_buffer = (int16_t *)malloc(max_samples * 2);
    if (audio_buffer == NULL) {
        return -1;
    }
    memset(audio_buffer, 0, max_samples * 2);

    /* ----------------------------------------------------------------
     * Initialize HDMI MAI (Multi-channel Audio Interconnect)
     *
     * The MAI bus connects the ARM DMA engine to the HDMI encoder's
     * audio input. Data written to HDMI_MAI_DATA is forwarded to the
     * HDMI IP block which embeds it in HDMI audio packets.
     * ---------------------------------------------------------------- */

    /* 1. Disable MAI and reset */
    MAI_REG(HDMI_MAI_CTL_OFFSET) = 0;
    usleep(100);

    /* 2. Set FIFO thresholds:
     *    High threshold (panic) = 0x10, Low threshold (dreq) = 0x08 */
    MAI_REG(HDMI_MAI_THR_OFFSET) = (0x10 << 16) | (0x08 << 8) | 0x08;

    /* 3. Set sample format: 16-bit stereo (2 channels) */
    MAI_REG(HDMI_MAI_FMT_OFFSET) = HDMI_MAI_FORMAT_16BIT | (2 - 1);

    /* 4. Set sample rate word (informational, written to HDMI infoframe) */
    MAI_REG(HDMI_MAI_SMP_OFFSET) = 0;   /* 0 = as per stream */

    /* 5. Enable MAI: 2 channels, whole-sample mode */
    MAI_REG(HDMI_MAI_CTL_OFFSET) = HDMI_MAI_CTL_EN
                                  | HDMI_MAI_CTL_WHOLSMP
                                  | HDMI_MAI_CTL_CHNUM(2);

    usleep(100);

    /* ----------------------------------------------------------------
     * Set up DMA Control Blocks.
     *
     * Source:      ARM memory buffer (bus address with L2 cache bypass)
     * Destination: HDMI MAI DATA FIFO (bus address 0x7E902000 + 0x01C)
     *
     * We use unpaced (PERMAP_0) DMA because the MAI FIFO is large
     * enough to accept a full audio buffer in one shot. Using DREQ
     * (paced) transfer requires the HDMI controller to assert DREQ on
     * a specific DMA channel which is not documented for bare-metal use.
     * ---------------------------------------------------------------- */
    for (int i = 0; i < 2; i++) {
        /* Unpaced DMA: source increments, destination fixed (FIFO) */
        dma_cb[i].ti        = HDMI_DMA_TI;
        /* Source: bus address with L2 bypass (0x40000000) for coherency */
        dma_cb[i].source_ad = 0x40000000 | (((uint32_t)dma_buffer[i] + 15) & ~0xF);
        /* Destination: MAI DATA FIFO bus address */
        dma_cb[i].dest_ad   = HDMI_BUS_BASE + HDMI_MAI_DATA_OFFSET;
        dma_cb[i].txfr_len  = max_samples * 4;
        dma_cb[i].stride    = 0;
        dma_cb[i].nextconbk = 0;
    }

    cur_buffer = 0;
    write_ptr  = (uint32_t *)((dma_cb[cur_buffer].source_ad & ~0x40000000));
    write_size = 0;

    return 0;
}

void audio_close() {
    IRQ->irq1Disable = INTERRUPT_DMA0;
    DMA->ch[0].cs = DMA_RESET;

    /* Disable MAI */
    MAI_REG(HDMI_MAI_CTL_OFFSET) = 0;

    for (int i = 0; i < 2; i++) {
        if (dma_buffer[i] != NULL) {
            free(dma_buffer[i]);
            dma_buffer[i] = NULL;
        }
    }
}

int audio_get_sample_rate() {
    return 22050;
}

int audio_get_channels() {
    return 2;
}

int audio_get_sample_size() {
    return 16 - SAMPLE_SHIFT;
}

void audio_play() {
    for (int i = 0; i < 2; i++) {
        memset(dma_buffer[i], 0, max_samples * 4 + 15);
    }

    IRQ->irq1Enable = INTERRUPT_DMA0;

    DMA->enable = DMA_EN0;
    DMA->ch[0].conblk_ad = 0x40000000 | (uint32_t)&dma_cb[cur_buffer];
    DMA->ch[0].cs = DMA_ACTIVE | DMA_INT;

    cur_buffer = cur_buffer == 0 ? 1 : 0;

    audio_callback(audio_buffer, max_samples);

    int16_t          * src = audio_buffer;
    volatile uint32_t * dst = (uint32_t *)((dma_cb[cur_buffer].source_ad & ~0x40000000));

    for (int i = 0; i < max_samples; i++) {
        /* HDMI MAI expects signed 16-bit PCM, packed as 32-bit words.
         * Shift up to fill 16-bit range (SAMPLE_SHIFT adjusts for
         * what the application puts in). */
        int16_t s = (int16_t)((*src++) << SAMPLE_SHIFT);
        /* Pack 16-bit signed sample as lower 16 bits, upper 16 = same (mono-expand)
         * or for stereo interleaved: even index = left, odd = right */
        *dst++ = (uint32_t)(uint16_t)s;
    }
}

void audio_stop() {
    IRQ->irq1Disable = INTERRUPT_DMA0;
}

void audio_dma_irq() {
    DMA->enable = DMA_EN0;
    DMA->ch[0].conblk_ad = 0x40000000 | (uint32_t)&dma_cb[cur_buffer];
    DMA->ch[0].cs = DMA_ACTIVE | DMA_INT;

    cur_buffer = cur_buffer == 0 ? 1 : 0;

    audio_callback(audio_buffer, max_samples);

    int16_t          * src = audio_buffer;
    volatile uint32_t * dst = (uint32_t *)((dma_cb[cur_buffer].source_ad & ~0x40000000));

    for (int i = 0; i < max_samples; i++) {
        int16_t s = (int16_t)((*src++) << SAMPLE_SHIFT);
        *dst++ = (uint32_t)(uint16_t)s;
    }
}

uint32_t audio_write(int16_t * stream, uint32_t samples) {
    uint32_t written = 0;

    while (write_size < max_samples && written < samples) {
        int16_t s = (int16_t)((*stream++) << SAMPLE_SHIFT);
        *write_ptr++ = (uint32_t)(uint16_t)s;
        write_size++;
        written++;

        if (write_size >= max_samples) {
            while ((DMA->ch[0].cs & DMA_ACTIVE) != 0)
                ;

            DMA->enable = DMA_EN0;
            DMA->ch[0].conblk_ad = 0x40000000 | (uint32_t)&dma_cb[cur_buffer];
            DMA->ch[0].cs = DMA_ACTIVE;

            cur_buffer = cur_buffer == 0 ? 1 : 0;
            write_ptr  = (uint32_t *)((dma_cb[cur_buffer].source_ad & ~0x40000000));
            write_size = 0;
        }
    }

    return written;
}

void audio_write_sample(int16_t sample) {
    int16_t s = (int16_t)(sample << SAMPLE_SHIFT);
    *write_ptr++ = (uint32_t)(uint16_t)s;
    write_size++;

    if (write_size >= max_samples) {
        while ((DMA->ch[0].cs & DMA_ACTIVE) != 0)
            ;

        DMA->enable = DMA_EN0;
        DMA->ch[0].conblk_ad = 0x40000000 | (uint32_t)&dma_cb[cur_buffer];
        DMA->ch[0].cs = DMA_ACTIVE;

        cur_buffer = cur_buffer == 0 ? 1 : 0;
        write_ptr  = (uint32_t *)((dma_cb[cur_buffer].source_ad & ~0x40000000));
        write_size = 0;
    }
}
