// Memory pool for allocations
alignas(16) char memory_pool[1024 * 1024]; // 1MB pool
size_t pool_index = 0;

// Implementation of operator new[] for array allocations
void *operator new[](size_t size)
{
    if (pool_index + size > sizeof(memory_pool))
    {
        return nullptr; // Out of memory
    }
    void *ptr = &memory_pool[pool_index];
    pool_index += size;
    return ptr;
}

// Implementation of operator delete[] (no-op for bump allocator)
void operator delete[](void *ptr) noexcept
{
    // Do nothing
}

// Function to read a byte from an I/O port
uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
};

// Memory map entry structure
struct mmap_entry
{
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

// External variable from boot.asm
extern "C" uint32_t mboot_info_ptr;

// Function to get total RAM in MB
uint32_t get_total_ram_mb(multiboot_info *mbi)
{
    // Check if memory info is available (bit 0 of flags)
    if (!(mbi->flags & 0x1))
    {
        return 0; // Memory info not available
    }

    // Basic memory info from multiboot
    uint32_t mem_kb = mbi->mem_lower + mbi->mem_upper;
    uint32_t mem_mb = mem_kb / 1024;

    // If memory map is available, we can get more accurate information
    if (mbi->flags & 0x40)
    {
        mem_mb = 0;
        mmap_entry *entry = (mmap_entry *)mbi->mmap_addr;
        mmap_entry *end = (mmap_entry *)((uint32_t)mbi->mmap_addr + mbi->mmap_length);

        while (entry < end)
        {
            // Type 1 indicates available RAM
            if (entry->type == 1)
            {
                mem_mb += (entry->len / 1048576); // Convert bytes to MB
            }
            // Go to next entry
            entry = (mmap_entry *)((uint32_t)entry + entry->size + 4);
        }
    }

    return mem_mb;
}