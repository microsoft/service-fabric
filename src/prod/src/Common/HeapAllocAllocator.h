// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <limits>
#undef max

namespace Common
{
    // trivial std::allocator wrapper around HeapAlloc 
    // (following the template from The C++ Standard Library - A Tutorial and Reference )
    // TODO: add support for HeapAlloc options, such as ZERO_INIT

    template <class T> class HeapAlloc_allocator;

    // specialize for void:
    template <> class HeapAlloc_allocator<void> {
    public:
        typedef void*       pointer;
        typedef const void* const_pointer;
        // reference to void members are impossible.
        typedef void value_type;
        template <class U> struct rebind { typedef HeapAlloc_allocator<U> other; };
    };

    template <class T>
    class HeapAlloc_allocator 
    {
    public:
        // type definitions
        typedef T        value_type;
        typedef T*       pointer;
        typedef const T* const_pointer;
        typedef T&       reference;
        typedef const T& const_reference;
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;

        // rebind allocator to type U
        template <class U> struct rebind { typedef HeapAlloc_allocator<U> other;   };

        // return address of values
        pointer address (reference value) const { return &value; }
        const_pointer address (const_reference value) const { return &value; }

        /* constructors and destructor
        * - nothing to do because the allocator has no state
        */
        HeapAlloc_allocator() throw() {}
        HeapAlloc_allocator(const HeapAlloc_allocator&) throw() { }

        template <class U> HeapAlloc_allocator (const HeapAlloc_allocator<U>&) throw() {}
        ~HeapAlloc_allocator() throw() {}

        // return maximum number of elements that can be allocated
        size_type max_size () const throw() { return std::numeric_limits<std::size_t>::max() / sizeof(T);  }

        // allocate but don't initialize num elements of type T
        pointer allocate (size_type num, const void* = 0) 
        {
            void* mem = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, num*sizeof(T));
            if (mem == nullptr)
                throw std::bad_alloc();           
            return reinterpret_cast<pointer>(mem);
        }

        // initialize elements of allocated storage p with value value
        template <class t0>
        void construct (pointer p, t0 && a0) 
        {
            // initialize memory with placement new
            new((void*)p)T(std::forward<t0>(a0));
        }

        template <class t0, class t1>
        void construct (pointer p, t0 && a0, t1 && a1) 
        {
            // initialize memory with placement new
            new((void*)p)T(std::forward<t0>(a0), std::forward<t1>(a1));
        }

        template <class t0, class t1, class t2>
        void construct (pointer p, t0 && a0, t1 && a1, t2 && a2) 
        {
            // initialize memory with placement new
            new((void*)p)T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2));
        }

        template <class t0, class t1, class t2, class t3>
        void construct(pointer p, t0 && a0, t1 && a1, t2 && a2, t3 && a3)
        {
            // initialize memory with placement new
            new((void*)p)T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3));
        }

        // destroy elements of initialized storage p
        void destroy (pointer p) {  p->~T();  }

        // deallocate storage p of deleted elements
        void deallocate (pointer p, size_type )
        {
            HeapFree(GetProcessHeap(), 0, p);
        }
    };

    // return that all specializations of this allocator are interchangeable
    template <class T1, class T2>
    bool operator== (const HeapAlloc_allocator<T1>&, const HeapAlloc_allocator<T2>&) throw() { return true; }

    template <class T1, class T2>
    bool operator!= (const HeapAlloc_allocator<T1>&, const HeapAlloc_allocator<T2>&) throw() { return false; }

    struct allocation_stats
    {
        static size_t deallocation_bytes;
        static size_t deallocation_count;
        static size_t allocation_bytes;
        static size_t allocation_count;

        static size_t leak_count() { return allocation_count - deallocation_count; }
        static size_t leak_bytes() { return allocation_bytes - deallocation_bytes; }
        
        static void WriteStats(Common::TextWriter &)
        {
            //w.WriteLine("total allocations {0} of size {1} bytes",
            //    allocation_count, allocation_bytes);

            //if (deallocation_bytes != allocation_bytes || deallocation_count != allocation_count)
            //{
            //    w.WriteLine(" ******* LEAKS ******** ");

            //    w.WriteLine("{0} leaks of size {1} bytes",
            //        allocation_count - deallocation_count, allocation_bytes - deallocation_bytes);
            //}
        }
        
    };
    __declspec(selectany) size_t allocation_stats::deallocation_bytes;
    __declspec(selectany) size_t allocation_stats::deallocation_count;
    __declspec(selectany) size_t allocation_stats::allocation_bytes;
    __declspec(selectany) size_t allocation_stats::allocation_count;

    template <class T>
    class leak_counting_allocator : public HeapAlloc_allocator<T>
    {
        typedef HeapAlloc_allocator<T> base_allocator;
    public:
        template <class U> struct rebind { typedef leak_counting_allocator<U> other;   };

        static size_t deallocation_count;
        static size_t allocation_count;

        typename HeapAlloc_allocator<T>::pointer allocate (typename HeapAlloc_allocator<T>::size_type num, const void* = 0) 
        {
            ++allocation_stats::allocation_count;
            ++allocation_count;
            allocation_stats::allocation_bytes += num * sizeof(T);
            return base_allocator::allocate(num);
        }

        void deallocate (typename HeapAlloc_allocator<T>::pointer p, typename HeapAlloc_allocator<T>::size_type num) 
        {
            ++deallocation_count;
            ++allocation_stats::deallocation_count;
            allocation_stats::deallocation_bytes += num * sizeof(T);

            base_allocator::deallocate (p, num);
        }

        static size_t leak_count() { return allocation_count - deallocation_count; }
    };

    template <class T> size_t leak_counting_allocator<T>::deallocation_count;
    template <class T> size_t leak_counting_allocator<T>::allocation_count;
}
