/* ac97.c - AC97 audio driver implementation */
#include "ac97.h"
#include "io.h"
#include "stddef.h"

/* Memory-mapped I/O base addresses */
static uint16_t nabm_base = 0;     /* Native Audio Bus Mastering (buffer control) */
static uint16_t nam_base = 0;      /* Native Audio Mixer (AC97 registers) */

/* Buffer descriptors */
static ac97_bd_t buffer_descriptors[AC97_BD_COUNT] __attribute__((aligned(8)));
static uint8_t *sound_buffer = NULL;
static uint32_t sound_buffer_size = 0;

/* For simplicity, we're using a hardcoded PCI configuration space access */
/* In a real driver, you'd properly scan the PCI bus */
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/* Simple delay function */
static void delay(int count) {
    for (volatile int i = 0; i < count * 1000; i++) {
        io_wait();
    }
}

/* Read a PCI configuration register */
static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | 
                                  (offset & 0xFC) | 0x80000000);
    
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

/* Find and initialize the AC97 controller */
bool ac97_init(void) {
    /* Look for the AC97 audio controller on all buses */
    /* This is a simplified PCI scan that works for most setups */
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor_device = pci_read_config(bus, slot, func, 0);
                
                /* Skip invalid devices (0xFFFFFFFF is returned for non-existent devices) */
                if (vendor_device == 0xFFFFFFFF) {
                    continue;
                }
                
                uint16_t vendor = vendor_device & 0xFFFF;
                uint16_t device = (vendor_device >> 16) & 0xFFFF;
                
                /* In VirtualBox, the AC97 device might be vendor 0x8086 (Intel) */
                /* But in reality, multiple vendors exist with different IDs */
                if ((vendor == 0x8086 && (device == 0x2415 || device == 0x2425 || device == 0x2445)) || 
                    (vendor == 0x1274 && (device == 0x1371 || device == 0x5880)) ||  /* Ensoniq/Creative */
                    (vendor == 0x1102 && (device == 0x0002 || device == 0x0004))) {  /* Creative */
                    
                    /* Read class code to verify it's an audio controller */
                    uint32_t class_code = pci_read_config(bus, slot, func, 0x08);
                    uint8_t base_class = (class_code >> 24) & 0xFF;
                    uint8_t sub_class = (class_code >> 16) & 0xFF;
                    
                    /* Check if this is an audio controller (base class 0x04, subclass 0x01) */
                    if (base_class == 0x04 && sub_class == 0x01) {
                        /* Found AC97 controller */
                        uint32_t bar0 = pci_read_config(bus, slot, func, 0x10); /* NAM BAR */
                        uint32_t bar1 = pci_read_config(bus, slot, func, 0x14); /* NABM BAR */
                        
                        /* Extract the I/O port base addresses (remove the lower bits) */
                        nam_base = (uint16_t)(bar0 & ~0xF);
                        nabm_base = (uint16_t)(bar1 & ~0xF);
                        
                        /* Check if we got valid I/O port addresses */
                        if (nam_base == 0 || nabm_base == 0) {
                            continue;
                        }
                        
                        /* Reset the AC97 controller */
                        outw(nam_base + AC97_RESET, 0);
                        delay(100); /* Wait for reset to complete */
                        
                        /* Set master volume to maximum (0 = max, 0x8000 = mute) */
                        outw(nam_base + AC97_MASTER_VOL, 0x0000);
                        outw(nam_base + AC97_PCM_OUT_VOL, 0x0000);
                        
                        /* Reset the bus master */
                        outb(nabm_base + AC97_PO_CR, AC97_CR_RR);
                        delay(10);
                        
                        /* Attempt to poll a register to verify hardware is working */
                        uint16_t vendor_id = inw(nam_base + 0x00);
                        if (vendor_id != 0xFFFF) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false; /* AC97 controller not found */
}

/* Play a sound buffer */
bool ac97_play_buffer(const uint8_t *buffer, uint32_t size) {
    if (!nam_base || !nabm_base) {
        return false; /* AC97 not initialized */
    }
    
    /* Stop any current playback */
    ac97_stop();
    
    /* Store buffer pointer for later reference */
    sound_buffer = (uint8_t *)buffer;
    sound_buffer_size = size;
    
    /* Set up buffer descriptors - use smaller chunks for better compatibility */
    uint32_t chunk_size = 16384; /* 16KB per buffer is safer for some implementations */
    uint32_t total_bds = (size + chunk_size - 1) / chunk_size;
    
    if (total_bds > AC97_BD_COUNT) {
        total_bds = AC97_BD_COUNT; /* Limit to available descriptors */
        chunk_size = (size + total_bds - 1) / total_bds; /* Adjust chunk size */
    }
    
    uint32_t remaining = size;
    uint32_t offset = 0;
    
    /* Make sure buffer descriptors are aligned and in a valid memory region */
    for (uint32_t i = 0; i < AC97_BD_COUNT; i++) {
        buffer_descriptors[i].buffer_addr = 0;
        buffer_descriptors[i].buffer_samples = 0;
        buffer_descriptors[i].buffer_control = 0;
    }
    
    for (uint32_t i = 0; i < total_bds; i++) {
        uint32_t this_chunk = (remaining > chunk_size) ? chunk_size : remaining;
        
        /* Buffer address must be physical and aligned */
        buffer_descriptors[i].buffer_addr = (uint32_t)(buffer + offset);
        
        /* Convert bytes to samples (16-bit samples) */
        buffer_descriptors[i].buffer_samples = this_chunk / 2;
        
        /* Set IOC (Interrupt on completion) on the last buffer */
        buffer_descriptors[i].buffer_control = (i == total_bds - 1) ? AC97_BD_IOC : 0;
        
        offset += this_chunk;
        remaining -= this_chunk;
    }
    
    /* Set the buffer descriptor list base address */
    outl(nabm_base + AC97_PO_BDBAR, (uint32_t)buffer_descriptors);
    
    /* Set the LVI (Last Valid Index) */
    outb(nabm_base + AC97_PO_LVI, total_bds - 1);
    
    /* Clear status bits */
    outw(nabm_base + AC97_PO_SR, 0x1C);
    
    /* Start playback */
    outb(nabm_base + AC97_PO_CR, AC97_CR_RPBM | AC97_CR_IOCE);
    
    /* Verify that playback has started */
    uint8_t status = inb(nabm_base + AC97_PO_CR);
    if (!(status & AC97_CR_RPBM)) {
        /* Try again if it didn't start */
        outb(nabm_base + AC97_PO_CR, AC97_CR_RPBM | AC97_CR_IOCE);
        delay(10);
    }
    
    return true;
}

/* Stop playback */
void ac97_stop(void) {
    if (nabm_base) {
        /* Stop the DMA engine */
        outb(nabm_base + AC97_PO_CR, 0);
        delay(10);
        
        /* Clear any pending interrupts */
        outw(nabm_base + AC97_PO_SR, 0x1C);
    }
}