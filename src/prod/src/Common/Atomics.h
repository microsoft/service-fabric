// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//#include <windows.h>

// http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2007/n2427.html
// implement only what you are using

namespace Common
{
    // add Acquire Release semantics if needed
    // 
    typedef enum memory_order {
            memory_order_relaxed, memory_order_acquire, memory_order_release,
            memory_order_acq_rel, memory_order_seq_cst
    } memory_order;

    class atomic_flag
    {
        atomic_flag( const atomic_flag& );// = delete; 
        atomic_flag& operator =( const atomic_flag& );// = delete;
    public:
        atomic_flag() : value_(0) {} // NON-STANDARD, standard requires trivial constructor

        bool test_and_set( ) volatile
        {
            return InterlockedExchange(&value_, 1) != 0;
        }

        void clear() volatile
        {
            InterlockedExchange(&value_, 0);
        }
    private:
        volatile LONG value_;
    };

    class atomic_bool
    {
        atomic_bool( const atomic_bool& ) ;  //= delete;
        atomic_bool& operator =( const atomic_bool & ) ; // = delete;
    public:
        bool is_lock_free() const volatile { return true; }
        void store( bool value ) volatile { exchange((LONG)value); }
        bool load( ) const volatile { return value_ != 0; }

        /// Returns: Atomically, the value of object immediately before the effects. 
        bool exchange( bool value ) volatile { return InterlockedExchange(&value_, (LONG)value) != 0; }
        bool compare_exchange_weak(bool& expected, bool desired) volatile
        {
            LONG prevValue = InterlockedCompareExchange(&value_, (LONG)desired, (LONG)expected);

            if (prevValue != (LONG)expected)
            {
                expected = (prevValue != 0);
                return false;
            }
            return true;
        }

        //If we move to ARM processors the implementation of strong remains the same.
        bool compare_exchange_strong(bool& expected, bool desired) volatile
        {
            LONG prevValue = InterlockedCompareExchange(&value_, (LONG)desired, (LONG)expected);

            if (prevValue != (LONG)expected)
            {
                expected = (prevValue != 0);
                return false;
            }
            return true;
        }
        void fence( memory_order ) const volatile;

        atomic_bool() : value_() {}
        explicit atomic_bool( bool value ) : value_(value) {}
        // bool operator =( bool ) volatile;
    private:
        volatile LONG value_;
    };

    class atomic_address
    {
        atomic_address( const atomic_address& ); // = delete;
        atomic_address& operator =( const atomic_address& ); // = delete;
    public:

        bool is_lock_free() const volatile { return true; }
        void store( void* newValue ) volatile { exchange(newValue); }

        // Effects: Atomically loads the value stored in *this. 
        void* load( ) const volatile { return value_; } // CR: should we put some barrier?
        void* exchange( void* newValue ) volatile { return InterlockedExchangePointer(&value_, newValue); }

        void fence( memory_order ) const volatile ; // implement when needed

        // Effects: Atomically retrieves the existing value of *this and stores old-value + add in *this. 
        //
        // Returns: The value of *this immediately prior to the store. 
        //
        // Throws: Nothing. 

        void* fetch_add( ptrdiff_t add ) volatile; // implement using integral fetch_sub when needed

        // Effects: Atomically retrieves the existing value of *this and stores old-value - sub in *this. 
        //
        // Returns: The value of *this immediately prior to the store. 
        //
        // Throws: Nothing. 
        void* fetch_sub( ptrdiff_t sub) volatile; // implement using integral fetch_sub when needed

        atomic_address() : value_(nullptr) {}
        /* constexpr */ explicit atomic_address( void* value ) : value_(value) {}

        // not sure about those, use explicit atomic operations instead
        // void* operator =( void* ) volatile; 
        // void* operator +=( ptrdiff_t ) volatile;
        // void* operator -=( ptrdiff_t ) volatile;
    private:
        void * volatile value_;
    };

    class atomic_long
    {
        atomic_long( const atomic_long& ); // = delete;
        atomic_long& operator =( const atomic_long& ); // = delete;
    public:

        bool is_lock_free() const volatile { return true; }
        void store( LONG newValue ) volatile { exchange(newValue); }

        // Effects: Atomically loads the value stored in *this. 
        LONG load( ) const volatile { return value_; } // CR: should we put some barrier?

        /// Effects: Atomically replace the value in object with desired. 
        ///          These operation is read-modify-write operation in the sense of the "synchronizes with" 
        ///          definition in [the new section added by N2334 or successor], and hence both such an operation 
        ///          and the evaluation that produced the input value synchronize with any evaluation that reads 
        ///          the updated value. 
        ///
        /// Returns: Atomically, the value of object immediately before the effects. 

        LONG exchange( LONG newValue ) volatile { return InterlockedExchange(&value_, newValue); }

        /// Effects: If *this is equivalent to expected, assigns desired to *this, 
        ///          otherwise assigns *this to expected.
        ///
        /// Returns: true if *this was equivalent to desired, false otherwise.
        ///
        /// Throws: nothing.

        bool compare_exchange_weak(LONG & expected, LONG desired)
        {
            LONG prevValue = InterlockedCompareExchange(&value_, desired, expected);

            if (prevValue != expected)
            {
                expected = prevValue;
                return false;
            }
            return true;
        }

        void fence( memory_order ) const volatile ; // implement when needed

        /// Effects: Atomically retrieves the existing value of *this and stores old-value + add in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 

        LONG fetch_add( LONG add ) volatile { return *this += add; }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value - sub in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        LONG fetch_sub( LONG sub) volatile { return *this -= sub; }

#if defined(_WIN64)

        /// Effects: Atomically retrieves the existing value of *this and stores old-value & r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        LONG fetch_and( LONG r ) volatile { return InterlockedAnd(&value_, r); }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value | r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        LONG fetch_or( LONG r ) volatile { return InterlockedOr(&value_, r); }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value xor r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        LONG fetch_xor( LONG r ) volatile { return InterlockedXor(&value_, r); }

#endif

        atomic_long() : value_() {}
        /* constexpr */ explicit atomic_long( LONG value ) : value_(value) {}

        // not sure about assignment, use explicit atomic_long operations instead
        // LONG operator =( LONG ) volatile; 
        LONG operator +=( LONG operand ) volatile
        {
            return InterlockedExchangeAdd(&value_, operand);
        }

        LONG operator -=( LONG operand ) volatile
        {
            return InterlockedExchangeAdd(&value_, -operand);
        }

        LONG operator ++( int ) volatile { return ++*this - 1; }
        LONG operator --( int ) volatile { return --*this + 1; }
        LONG operator ++() volatile { return InterlockedIncrement(&value_); }
        LONG operator --() volatile { return InterlockedDecrement(&value_); }

    private:
        LONG volatile value_;
    };

    class atomic_uint64
    {
        atomic_uint64( const atomic_uint64& ); // = delete;
        atomic_uint64& operator =( const atomic_uint64& ); // = delete;
    public:

        bool is_lock_free() const volatile { return true; }
        void store( uint64 newValue ) volatile { exchange(newValue); }

        // Effects: Atomically loads the value stored in *this. 
        uint64 load( ) const volatile { return value_; } // CR: should we put some barrier?

        /// Effects: Atomically replace the value in object with desired. 
        ///          These operation is read-modify-write operation in the sense of the "synchronizes with" 
        ///          definition in [the new section added by N2334 or successor], and hence both such an operation 
        ///          and the evaluation that produced the input value synchronize with any evaluation that reads 
        ///          the updated value. 
        ///
        /// Returns: Atomically, the value of object immediately before the effects. 

        uint64 exchange( uint64 newValue ) volatile { return InterlockedExchange64((volatile LONGLONG*)&value_, newValue); }

        void fence( memory_order ) const volatile ; // implement when needed

        /// Effects: Atomically retrieves the existing value of *this and stores old-value + add in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 

        uint64 fetch_add( uint64 add ) volatile { return *this += add; }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value - sub in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        uint64 fetch_sub( uint64 sub) volatile { return *this -= sub; }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value & r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        uint64 fetch_and( uint64 r ) volatile { return InterlockedAnd64((volatile LONGLONG*)&value_, r); }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value | r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        uint64 fetch_or( uint64 r ) volatile { return InterlockedOr64((volatile LONGLONG*)&value_, r); }

        /// Effects: Atomically retrieves the existing value of *this and stores old-value xor r in *this. 
        ///
        /// Returns: The value of *this immediately prior to the store. 
        ///
        /// Throws: Nothing. 
        uint64 fetch_xor( uint64 r ) volatile { return InterlockedXor64((volatile LONGLONG*)&value_, r); }

        atomic_uint64() : value_() {}
        /* constexpr */ explicit atomic_uint64( uint64 value ) : value_(value) {}

        // not sure about assignment, use explicit atomic_uint64 operations instead
        // uint64 operator =( uint64 ) volatile; 
        uint64 operator +=( uint64 operand ) volatile
        {
            return InterlockedExchangeAdd64((volatile LONGLONG*)&value_, operand);
        }

        uint64 operator -=( uint64 operand ) volatile
        {
            return InterlockedExchangeSubtract(&value_, operand);
        }

        uint64 operator |=( uint64 operand ) volatile { return fetch_or(operand) | operand; }

        uint64 operator &=( uint64 operand ) volatile { return fetch_and(operand) & operand; }

        uint64 operator ^=( uint64 operand ) volatile { return fetch_xor(operand) ^ operand; }

        uint64 operator ++( int ) volatile { return ++*this - 1; }
        uint64 operator --( int ) volatile { return --*this + 1; }
        uint64 operator ++() volatile { return InterlockedIncrement64((volatile LONGLONG*)&value_); }
        uint64 operator --() volatile { return InterlockedDecrement64((volatile LONGLONG*)&value_); }

    private:
        uint64 volatile value_;
    };


    template< class T > //  requires AtomicComparable< T >
    struct atomic
    {
        //bool is_lock_free() const volatile;
        //void store( T, memory_order = memory_order_seq_cst ) volatile;
        //T load( memory_order = memory_order_seq_cst ) volatile;
        //T exchange( T, memory_order = memory_order_seq_cst ) volatile;
        //void fence( memory_order ) const volatile;

        //atomic() = default;
        //constexpr explicit atomic( T );
        //atomic( const atomic& ) = delete;
        //atomic& operator =( const atomic& ) = delete;
        //T operator =( T ) volatile;
    };

    template <> class atomic<bool> : public atomic_bool
    {
    public:
        atomic() {}
        atomic(bool value) : atomic_bool(value) {}
    };

    template <> class atomic<LONG> : public atomic_long
    {
    public:
        atomic() {}
        atomic(LONG value) : atomic_long(value) {}
    };

    template<typename T>
    class atomic< T* > : atomic_address
    {
        atomic( const atomic& ) ; // = delete;
        atomic& operator =( const atomic& ) ; // = delete;
    public:
        T* fetch_add( ptrdiff_t add) volatile { return reinterpret_cast<T*>( atomic_address::fetch_add(add) ); }
        T* fetch_sub( ptrdiff_t sub) volatile { return reinterpret_cast<T*>( atomic_address::fetch_sub(sub) ); }

        void store( T* newValue ) volatile { atomic_address::exchange(newValue); }

        // Effects: Atomically loads the value stored in *this. 
        T* load( ) const volatile { return reinterpret_cast<T*>( atomic_address::load() ); } 
        T* exchange( T* newValue ) volatile { return reinterpret_cast<T*>( atomic_address::exchange(newValue) ); }

        atomic() {}
        /* constexpr */ explicit atomic( T* value ) : atomic_address (value) {}

        // use explicit atomic functions for those
        //T* operator =( T* ) volatile;
        //T* operator ++( int ) volatile;
        //T* operator --( int ) volatile;
        //T* operator ++() volatile;
        //T* operator --() volatile;
        //T* operator +=( ptrdiff_t ) volatile;
        //T* operator -=( ptrdiff_t ) volatile;
    };
};

