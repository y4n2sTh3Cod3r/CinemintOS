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