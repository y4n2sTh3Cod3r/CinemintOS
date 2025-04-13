// Define a size type since we can't use std::size_t
using size_t = unsigned int;

// Custom placement new and delete operators
inline void *operator new(size_t, void *p) noexcept { return p; }
inline void operator delete(void *, void *) noexcept {}

template <typename T>
class vector
{
private:
    T *arr;              // Pointer to the dynamic array
    size_t current_size; // Number of elements currently stored
    size_t cap;          // Total allocated capacity

    // Helper function to destroy all constructed elements
    void destroy_elements()
    {
        for (size_t i = 0; i < current_size; ++i)
        {
            arr[i].~T();
        }
    }

public:
    // Default constructor: initialize with capacity 10
    vector() : current_size(0), cap(10)
    {
        char *raw = new char[sizeof(T) * cap];
        arr = reinterpret_cast<T *>(raw);
    }

    // Destructor: clean up elements and memory
    ~vector()
    {
        destroy_elements();
        delete[] reinterpret_cast<char *>(arr);
    }

    // Add an element to the end
    void push_back(const T &value)
    {
        if (current_size == cap)
        {
            size_t new_cap = cap * 2;
            char *new_raw = new char[sizeof(T) * new_cap];
            T *new_arr = reinterpret_cast<T *>(new_raw);
            for (size_t i = 0; i < current_size; ++i)
            {
                new (new_arr + i) T(arr[i]); // Copy existing elements
            }
            destroy_elements();
            delete[] reinterpret_cast<char *>(arr);
            arr = new_arr;
            cap = new_cap;
        }
        new (arr + current_size) T(value); // Construct new element
        ++current_size;
    }

    // Remove the last element
    void pop_back()
    {
        if (current_size > 0)
        {
            --current_size;
            arr[current_size].~T(); // Destroy the last element
        }
    }

    // Return the number of elements
    size_t size() const
    {
        return current_size;
    }

    // Return the current capacity
    size_t capacity() const
    {
        return cap;
    }

    bool empty() const
    {
        return current_size == 0 ? true : false;
    }

    // Element access (non-const)
    T &operator[](size_t index)
    {
        return arr[index];
    }

    // Element access (const)
    const T &operator[](size_t index) const
    {
        return arr[index];
    }

    // Copy constructor
    vector(const vector &other) : current_size(other.current_size), cap(other.cap)
    {
        char *raw = new char[sizeof(T) * cap];
        arr = reinterpret_cast<T *>(raw);
        for (size_t i = 0; i < current_size; ++i)
        {
            new (arr + i) T(other.arr[i]); // Deep copy elements
        }
    }

    // Copy assignment operator
    vector &operator=(const vector &other)
    {
        if (this != &other)
        {
            destroy_elements();
            delete[] reinterpret_cast<char *>(arr);
            current_size = other.current_size;
            cap = other.cap;
            char *raw = new char[sizeof(T) * cap];
            arr = reinterpret_cast<T *>(raw);
            for (size_t i = 0; i < current_size; ++i)
            {
                new (arr + i) T(other.arr[i]);
            }
        }
        return *this;
    }

    // Move constructor
    vector(vector &&other) : arr(other.arr), current_size(other.current_size), cap(other.cap)
    {
        other.arr = nullptr;
        other.current_size = 0;
        other.cap = 0;
    }

    // Move assignment operator
    vector &operator=(vector &&other)
    {
        if (this != &other)
        {
            destroy_elements();
            delete[] reinterpret_cast<char *>(arr);
            arr = other.arr;
            current_size = other.current_size;
            cap = other.cap;
            other.arr = nullptr;
            other.current_size = 0;
            other.cap = 0;
        }
        return *this;
    }

    // Remove all elements (keep capacity)
    void clear()
    {
        destroy_elements();
        current_size = 0;
    }

    // Resize the vector to n elements
    void resize(size_t n)
    {
        if (n > cap)
        {
            size_t new_cap = n > 2 * cap ? n : 2 * cap;
            char *new_raw = new char[sizeof(T) * new_cap];
            T *new_arr = reinterpret_cast<T *>(new_raw);
            for (size_t i = 0; i < current_size; ++i)
            {
                new (new_arr + i) T(arr[i]); // Copy existing elements
            }
            for (size_t i = current_size; i < n; ++i)
            {
                new (new_arr + i) T(); // Default construct new elements
            }
            destroy_elements();
            delete[] reinterpret_cast<char *>(arr);
            arr = new_arr;
            cap = new_cap;
            current_size = n;
        }
        else if (n > current_size)
        {
            for (size_t i = current_size; i < n; ++i)
            {
                new (arr + i) T(); // Default construct additional elements
            }
            current_size = n;
        }
        else
        {
            for (size_t i = n; i < current_size; ++i)
            {
                arr[i].~T(); // Destroy excess elements
            }
            current_size = n;
        }
    }

    // Reserve capacity for at least n elements
    void reserve(size_t n)
    {
        if (n > cap)
        {
            char *new_raw = new char[sizeof(T) * n];
            T *new_arr = reinterpret_cast<T *>(new_raw);
            for (size_t i = 0; i < current_size; ++i)
            {
                new (new_arr + i) T(arr[i]); // Copy existing elements
            }
            destroy_elements();
            delete[] reinterpret_cast<char *>(arr);
            arr = new_arr;
            cap = n;
        }
    }

    // Iterator support
    T *begin()
    {
        return arr;
    }

    T *end()
    {
        return arr + current_size;
    }

    const T *begin() const
    {
        return arr;
    }

    const T *end() const
    {
        return arr + current_size;
    }
};