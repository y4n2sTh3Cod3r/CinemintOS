/* ac97.h - AC97 audio driver header */
#ifndef AC97_H
#define AC97_H

#include "stdint.h"
#include "stdbool.h"

/* AC97 PCI device vendor/device ID - broader range to support more hardware */
#define AC97_VENDOR_ID 0x8086      /* Intel */
#define AC97_DEVICE_ID 0x2415      /* Generic Intel AC97 */

/* VMware Sound device */
#define VMWARE_VENDOR_ID 0x15AD
#define VMWARE_DEVICE_ID 0x1977    /* One of VMware's sound device IDs */

/* Ensoniq/Creative */
#define ENSONIQ_VENDOR_ID 0x1274
#define CREATIVE_VENDOR_ID 0x1102

/* AC97 Bus Master registers (relative to NABM BAR) */
#define AC97_PO_BDBAR     0x10    /* PCM Out buffer descriptor BAR */
#define AC97_PO_CIV       0x14    /* PCM Out current index value */
#define AC97_PO_LVI       0x15    /* PCM Out last valid index */
#define AC97_PO_SR        0x16    /* PCM Out status register */
#define AC97_PO_PICB      0x18    /* PCM Out position in current buffer */
#define AC97_PO_CR        0x1B    /* PCM Out control register */

/* AC97 Mixer registers (relative to NAMBAR) */
#define AC97_RESET        0x00    /* Reset register */
#define AC97_MASTER_VOL   0x02    /* Master volume */
#define AC97_PCM_OUT_VOL  0x18    /* PCM output volume */

/* Status register bits */
#define AC97_SR_DCH       (1<<0)  /* DMA Controller Halted */
#define AC97_SR_CELV      (1<<1)  /* Current Equals Last Valid */
#define AC97_SR_LVBCI     (1<<2)  /* Last Valid Buffer Completion Interrupt */
#define AC97_SR_BCIS      (1<<3)  /* Buffer Completion Interrupt Status */
#define AC97_SR_FIFOE     (1<<4)  /* FIFO Error */

/* Control register bits */
#define AC97_CR_RPBM      (1<<0)  /* Run/Pause Bus Master */
#define AC97_CR_RR        (1<<1)  /* Reset Registers */
#define AC97_CR_LVBIE     (1<<2)  /* Last Valid Buffer Interrupt Enable */
#define AC97_CR_FEIE      (1<<3)  /* FIFO Error Interrupt Enable */
#define AC97_CR_IOCE      (1<<4)  /* Interrupt On Completion Enable */

/* Buffer descriptor structure */
typedef struct {
    uint32_t buffer_addr;          /* Physical address of buffer */
    uint16_t buffer_samples;       /* Buffer size in samples (bytes / 2) */
    uint16_t buffer_control;       /* Buffer control flags */
} __attribute__((packed)) ac97_bd_t;

/* Buffer control bits */
#define AC97_BD_IOC       (1<<15)  /* Interrupt on completion */
#define AC97_BD_BUP       (1<<14)  /* Buffer underrun policy */

/* Maximum number of buffer descriptors */
#define AC97_BD_COUNT     32

/* Initialize the AC97 audio controller */
bool ac97_init(void);

/* Play a PCM sound buffer */
bool ac97_play_buffer(const uint8_t *buffer, uint32_t size);

/* Stop playback */
void ac97_stop(void);

#endif /* AC97_H */