#ifndef AC97_DRIVER_H
#define AC97_DRIVER_H

#include <stdint.h>

namespace cm {

// PCI Configuration Space access
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// PCI Class codes
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_SUBCLASS_MULTIMEDIA_AUDIO 0x01

// AC97 specific PCI information
#define AC97_VENDOR_ID_INTEL 0x8086
#define AC97_DEVICE_ID_INTEL 0x2415  // Intel 82801AA AC'97 Audio Controller

// AC97 controller registers
#define AC97_NABM_BASE 0x0000  // Native Audio Bus Mastering base address
#define AC97_MIXER_BASE 0x0100 // AC97 mixer register base address

// PCM OUT registers (playback)
#define AC97_PO_BDBAR 0x10 // PCM Out Buffer Descriptor BAR
#define AC97_PO_CIV 0x14   // PCM Out Current Index Value
#define AC97_PO_LVI 0x15   // PCM Out Last Valid Index
#define AC97_PO_SR 0x16    // PCM Out Status Register
#define AC97_PO_PICB 0x18  // PCM Out Position in Current Buffer
#define AC97_PO_CR 0x1B    // PCM Out Control Register

// Control register bits
#define AC97_X_CR_RPBM 0x01  // Run/Pause Bus Master
#define AC97_X_CR_RR 0x02    // Reset Registers
#define AC97_X_CR_LVBIE 0x04 // Last Valid Buffer Interrupt Enable
#define AC97_X_CR_FEIE 0x08  // FIFO Error Interrupt Enable
#define AC97_X_CR_IOCE 0x10  // Interrupt On Completion Enable

// Status register bits
#define AC97_X_SR_DCH 0x01   // DMA Controller Halted
#define AC97_X_SR_CELV 0x02  // Current Equals Last Valid
#define AC97_X_SR_LVBCI 0x04 // Last Valid Buffer Completion Interrupt
#define AC97_X_SR_BCIS 0x08  // Buffer Completion Interrupt Status
#define AC97_X_SR_FIFOE 0x10 // FIFO Error

// AC97 mixer registers
#define AC97_RESET 0x00       // Reset register
#define AC97_MASTER_VOL 0x02  // Master volume
#define AC97_PCM_OUT_VOL 0x18 // PCM output volume

// Function to read a 32-bit value from an I/O port
static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Function to write a 32-bit value to an I/O port
static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

// Function to read PCI configuration space
static uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(1 << 31 | 
                                bus << 16 | 
                                device << 11 | 
                                function << 8 | 
                                (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Buffer Descriptor List entry
struct ac97_bd {
    uint32_t buffer_addr;    // Buffer physical address
    uint16_t buffer_samples; // Buffer size in samples (multiple of 2)
    uint16_t control;        // Control flags
} __attribute__((packed));

// AC97 buffer descriptor control flags
#define AC97_BD_IOC 0x8000 // Interrupt on completion
#define AC97_BD_BUP 0x4000 // Buffer underrun policy (0: no repeat, 1: repeat)

class AC97Driver {
private:
    // PCI bus information
    uint8_t pci_bus;
    uint8_t pci_device;
    uint8_t pci_function;
    
    // I/O base addresses
    uint16_t nabm_base;
    uint16_t mixer_base;
    
    // Buffer descriptor list
    static const int BD_COUNT = 32;
    ac97_bd buffer_descriptors[BD_COUNT] __attribute__((aligned(8)));
    
    // Audio buffers
    static const int BUFFER_SAMPLES = 1024;               // Buffer size in samples
    uint16_t audio_buffers[BD_COUNT][BUFFER_SAMPLES * 2]; // 2 channels for stereo
    
    uint8_t current_buffer;
    bool initialized;
    
    // Read/write NABM registers
    uint8_t read_nabm8(uint8_t reg) {
        return inb(nabm_base + reg);
    }
    
    uint16_t read_nabm16(uint8_t reg) {
        return inw(nabm_base + reg);
    }
    
    uint32_t read_nabm32(uint8_t reg) {
        return inl(nabm_base + reg);
    }
    
    void write_nabm8(uint8_t reg, uint8_t val) {
        outb(nabm_base + reg, val);
    }
    
    void write_nabm16(uint8_t reg, uint16_t val) {
        outw(nabm_base + reg, val);
    }
    
    void write_nabm32(uint8_t reg, uint32_t val) {
        outl(nabm_base + reg, val);
    }
    
    // Read/write AC97 mixer registers
    uint16_t read_mixer(uint8_t reg) {
        return inw(mixer_base + reg);
    }
    
    void write_mixer(uint8_t reg, uint16_t val) {
        outw(mixer_base + reg, val);
    }
    
    // Find the AC97 controller on the PCI bus
    bool detect_hardware() {
        for (uint16_t bus = 0; bus < 256; bus++) {
            for (uint16_t device = 0; device < 32; device++) {
                for (uint16_t function = 0; function < 8; function++) {
                    // Read class/subclass
                    uint32_t class_info = pci_read_config(bus, device, function, 0x08);
                    uint8_t class_code = (class_info >> 24) & 0xFF;
                    uint8_t subclass_code = (class_info >> 16) & 0xFF;
                    
                    if (class_code == PCI_CLASS_MULTIMEDIA && subclass_code == PCI_SUBCLASS_MULTIMEDIA_AUDIO) {
                        // Found an audio device, check if it's an AC97
                        uint32_t vendor_device = pci_read_config(bus, device, function, 0x00);
                        uint16_t vendor_id = vendor_device & 0xFFFF;
                        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
                        
                        // This is a simplified check - you might want to add more known AC97 controllers
                        if (vendor_id == AC97_VENDOR_ID_INTEL) {
                            // Save the PCI information
                            pci_bus = bus;
                            pci_device = device;
                            pci_function = function;
                            
                            // Read the I/O base addresses from BAR0 and BAR1
                            uint32_t bar0 = pci_read_config(bus, device, function, 0x10);
                            uint32_t bar1 = pci_read_config(bus, device, function, 0x14);
                            
                            // Check if these are I/O addresses (bit 0 set)
                            if ((bar0 & 1) && (bar1 & 1)) {
                                // Get the base I/O addresses
                                nabm_base = (uint16_t)(bar0 & 0xFFFC);
                                mixer_base = (uint16_t)(bar1 & 0xFFFC);
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        // If we don't find it via PCI, use default addresses
        // This is for testing in some VM environments
        nabm_base = AC97_NABM_BASE;
        mixer_base = AC97_MIXER_BASE;
        
        return true;
    }
    
public:
    AC97Driver() : pci_bus(0), pci_device(0), pci_function(0),
                  nabm_base(0), mixer_base(0), current_buffer(0), initialized(false) {}
    
    bool init() {
        // First, detect the hardware
        if (!detect_hardware()) {
            return false;
        }
        
        // Reset the AC97 codec
        write_mixer(AC97_RESET, 1);
        
        // Wait for reset to complete
        for (int i = 0; i < 1000; i++) {
            if (read_mixer(AC97_RESET) & 0x01) {
                // Reset successful
                break;
            }
            
            // Simple delay
            for (volatile int j = 0; j < 10000; j++);
        }
        
        // Set master volume (0 = max volume, 0x8000 = mute)
        write_mixer(AC97_MASTER_VOL, 0x0000);
        
        // Set PCM output volume
        write_mixer(AC97_PCM_OUT_VOL, 0x0000);
        
        // Initialize buffer descriptors
        for (int i = 0; i < BD_COUNT; i++) {
            buffer_descriptors[i].buffer_addr = (uint32_t)&audio_buffers[i][0];
            buffer_descriptors[i].buffer_samples = BUFFER_SAMPLES * 4; // 4 bytes per sample (16-bit stereo)
            buffer_descriptors[i].control = AC97_BD_IOC; // Interrupt on completion
            
            // Clear audio buffer
            for (int j = 0; j < BUFFER_SAMPLES * 2; j++) {
                audio_buffers[i][j] = 0;
            }
        }
        
        // Reset the DMA engine
        write_nabm8(AC97_PO_CR, AC97_X_CR_RR);
        
        // Wait for reset to complete
        for (int i = 0; i < 1000; i++) {
            if ((read_nabm8(AC97_PO_CR) & AC97_X_CR_RR) == 0) {
                break;
            }
            
            // Simple delay
            for (volatile int j = 0; j < 10000; j++);
        }
        
        // Set buffer descriptor list base address
        write_nabm32(AC97_PO_BDBAR, (uint32_t)buffer_descriptors);
        
        // Set last valid index
        write_nabm8(AC97_PO_LVI, BD_COUNT - 1);
        
        // Enable interrupts
        write_nabm8(AC97_PO_CR, AC97_X_CR_FEIE | AC97_X_CR_IOCE);
        
        initialized = true;
        return true;
    }
    
    void start() {
        if (!initialized)
            return;
        
        // Start DMA
        write_nabm8(AC97_PO_CR, read_nabm8(AC97_PO_CR) | AC97_X_CR_RPBM);
    }
    
    void stop() {
        if (!initialized)
            return;
        
        // Stop DMA
        write_nabm8(AC97_PO_CR, read_nabm8(AC97_PO_CR) & ~AC97_X_CR_RPBM);
        
        // Wait until DMA halts
        while ((read_nabm8(AC97_PO_SR) & AC97_X_SR_DCH) == 0);
    }
    
    bool is_playing() {
        return initialized && ((read_nabm8(AC97_PO_SR) & AC97_X_SR_DCH) == 0);
    }
    
    // Fill a buffer with PCM audio data
    bool fill_buffer(uint8_t buffer_index, const void *data, uint16_t size, bool is_16bit, bool is_stereo) {
        if (!initialized || buffer_index >= BD_COUNT)
            return false;
        
        // Copy data to buffer
        if (is_16bit) {
            if (is_stereo) {
                // 16-bit stereo
                const int16_t *src = (const int16_t *)data;
                uint16_t *dst = &audio_buffers[buffer_index][0];
                uint16_t samples = size / 4; // 4 bytes per sample (16-bit stereo)
                
                for (int i = 0; i < samples; i++) {
                    dst[i * 2] = src[i * 2];         // Left channel
                    dst[i * 2 + 1] = src[i * 2 + 1]; // Right channel
                }
            } else {
                // 16-bit mono (duplicate to both channels)
                const int16_t *src = (const int16_t *)data;
                uint16_t *dst = &audio_buffers[buffer_index][0];
                uint16_t samples = size / 2; // 2 bytes per sample (16-bit mono)
                
                for (int i = 0; i < samples; i++) {
                    dst[i * 2] = src[i];     // Left channel
                    dst[i * 2 + 1] = src[i]; // Right channel
                }
            }
        } else {
            // 8-bit (convert to 16-bit)
            if (is_stereo) {
                // 8-bit stereo
                const uint8_t *src = (const uint8_t *)data;
                uint16_t *dst = &audio_buffers[buffer_index][0];
                uint16_t samples = size / 2; // 2 bytes per sample (8-bit stereo)
                
                for (int i = 0; i < samples; i++) {
                    // Convert 8-bit unsigned to 16-bit signed
                    dst[i * 2] = ((int16_t)src[i * 2] - 128) << 8;         // Left channel
                    dst[i * 2 + 1] = ((int16_t)src[i * 2 + 1] - 128) << 8; // Right channel
                }
            } else {
                // 8-bit mono (duplicate to both channels)
                const uint8_t *src = (const uint8_t *)data;
                uint16_t *dst = &audio_buffers[buffer_index][0];
                uint16_t samples = size; // 1 byte per sample (8-bit mono)
                
                for (int i = 0; i < samples; i++) {
                    // Convert 8-bit unsigned to 16-bit signed
                    int16_t sample = ((int16_t)src[i] - 128) << 8;
                    dst[i * 2] = sample;     // Left channel
                    dst[i * 2 + 1] = sample; // Right channel
                }
            }
        }
        
        return true;
    }
    
    // Get next available buffer index
    uint8_t get_next_buffer() {
        if (!initialized)
            return 0;
        
        // Get current index value from hardware
        uint8_t civ = read_nabm8(AC97_PO_CIV);
        
        // Calculate next buffer (at least 2 ahead of CIV to avoid race conditions)
        return (civ + 2) % BD_COUNT;
    }
};

// Global AC97 driver instance
AC97Driver ac97;

// Helper function to play WAV with AC97
bool play_wav_ac97(const void *samples, uint32_t sampleRate, uint32_t numSamples,
                  uint8_t bitsPerSample, uint8_t channels) {
    if (!ac97.is_playing()) {
        // Initialize AC97 if not already
        if (!ac97.init()) {
            return false;
        }
        
        // Fill initial buffers with WAV data
        const uint8_t *data = (const uint8_t *)samples;
        bool is_16bit = (bitsPerSample == 16);
        bool is_stereo = (channels == 2);
        
        // Calculate bytes per sample
        uint8_t bytesPerSample = bitsPerSample / 8 * channels;
        
        // Fill all buffers
        for (int i = 0; i < 32; i++) {
            uint32_t offset = i * 1024 * bytesPerSample;
            uint32_t size = 1024 * bytesPerSample;
            
            // Check if we reach the end of the data
            if (offset >= numSamples * bytesPerSample) {
                // No more data to fill
                break;
            }
            
            // Adjust size if we reach the end
            if (offset + size > numSamples * bytesPerSample) {
                size = numSamples * bytesPerSample - offset;
            }
            
            ac97.fill_buffer(i, (const uint8_t *)data + offset, size, is_16bit, is_stereo);
        }
        
        // Start playback
        ac97.start();
        return true;
    }
    
    return false;
}

} // namespace cm

#endif // AC97_DRIVER_H