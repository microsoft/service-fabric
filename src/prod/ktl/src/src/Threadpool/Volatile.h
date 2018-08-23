#pragma once

namespace KtlThreadpool{

    #define VOLATILE_MEMORY_BARRIER() asm volatile ("" : : : "memory")

    template<typename T> inline T VolatileLoad(T const * pt)
    {
        T val = *(T volatile const *)pt;
        VOLATILE_MEMORY_BARRIER();
        return val;
    }

    template<typename T> inline void VolatileStore(T* pt, T val)
    {
        VOLATILE_MEMORY_BARRIER();
        *(T volatile *)pt = val;
    }

    template <typename T> class Volatile {
    private:
        T m_val;

    public:
        inline Volatile()
        {
        }

        inline Volatile(const T &val)
        {
            ((volatile T &) m_val) = val;
        }

        inline Volatile(const Volatile<T> &other)
        {
            ((volatile T &) m_val) = other.Load();
        }

        inline T Load() const
        {
            return VolatileLoad(&m_val);
        }

        inline T LoadWithoutBarrier() const
        {
            return ((volatile T &) m_val);
        }

        inline void Store(const T& val)
        {
            VolatileStore(&m_val, val);
        }

        inline void StoreWithoutBarrier(const T& val) const
        {
            ((volatile T &)m_val) = val;
        }

        inline volatile T* GetPointer() { return (volatile T*)&m_val; }

        inline T& RawValue() { return m_val; }

        inline operator T() const
        {
            return this->Load();
        }

        inline Volatile<T>& operator=(T val) {Store(val); return *this;}

        inline T volatile * operator&() { return this->GetPointer(); }
        inline T volatile const * operator&() const { return this->GetPointer(); }
        template<typename TOther> inline bool operator==(const TOther& other) const { return this->Load() == other; }
        template<typename TOther> inline bool operator!=(const TOther& other) const { return this->Load() != other; }
        inline Volatile<T>& operator+=(T val) { Store(this->Load() + val); return *this; }
        inline Volatile<T>& operator-=(T val) { Store(this->Load() - val); return *this; }
        inline Volatile<T>& operator|=(T val) { Store(this->Load() | val); return *this; }
        inline Volatile<T>& operator&=(T val) { Store(this->Load() & val); return *this; }
        inline bool operator!() const { return !this->Load(); }
        inline Volatile& operator++() { this->Store(this->Load()+1); return *this; }
        inline T operator++(int) { T val = this->Load(); this->Store(val+1); return val; }
        inline Volatile& operator--() { this->Store(this->Load()-1); return *this; }
        inline T operator--(int) { T val = this->Load(); this->Store(val-1); return val; }
    };
}
