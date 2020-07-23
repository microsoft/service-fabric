/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KArray

    Description:
      Kernel Template Library (KTL): KArray

      Implements a general purpose self-expanding array
      Also defines a macro KSharedArray to have KShared array's

    History:
      raymcc          16-Aug-2010         Definition
      raymcc          02-Sep-2010         Initial implementation.

--*/

#pragma once

// KArray can be instantiated with a type that has overloaded operator &.
// __builtin_addressof is used to take the address to avoid issues with the overloaded operator.
// __builtin_addressof is implemented in both VC and Clang/GCC.

// Temporary enable/disable for blittable support. When enabled, the behavior of some functions will be modified to
// do direct memory copy (blit) of array regions when supported by the element type. When disabled, elements will 
// be copied one-at-a-time by assignment. Enabling this feature should increase perf dramatically when e.g. using
// RemoveRange on an array of blittable elements.
//#undef  KArrayImpBlittableType         // DISABLE
#define KArrayImpBlittableType 1         // ENABLE

//
// Trait that can be used to check if type T is decorated with the KIsBlittableType() or not
//
template <class T, class = int>
struct isBlittable
{
    static constexpr bool value = false;
};

template <class T>
struct isBlittable<T, decltype((void)T::KIsBlittableType, 0)>
{
    static constexpr bool value = true;
};

template <class T, class = int>
struct isBlittableDefaultIsZero
{
    static constexpr bool value = false;
};

template <class T>
struct isBlittableDefaultIsZero<T, decltype((void)T::KIsBlittableType_DefaultIsZero, 0)>
{
    static constexpr bool value = true;
};

template <typename T, class = int>
struct KArrayFactory
{
    static T* ConstructArray(ULONG count, KAllocator& allocator)
    {
        T* tmp = _newArray<T>(KTL_TAG_ARRAY, allocator, count);
        return tmp;
    }
    static void Construct(T& t, KAllocator&)
    {
        new(__builtin_addressof(t)) T();
    }
    static T Construct(KAllocator&)
    {
        return T();
    }
    static void DestructArray(T* p, ULONG)
    {
        _deleteArray(p);
    }
};

template <typename T>
struct KArrayFactory<T, decltype((void)T::KAllocatorRequired, 0)>
{
    static T* ConstructArray(ULONG count, KAllocator& allocator)
    {
        HRESULT hr;
        ULONG result;
        hr = ULongMult(count, sizeof(T), &result);
        KInvariant(SUCCEEDED(hr));
        T* tmp = (T*)(_newArray<UCHAR>(KTL_TAG_ARRAY, allocator, result));
        if (!tmp)
        {
            return nullptr;
        }

        // In-place CTOR for T instances passing their allocator
        for (ULONG ix = 0; ix < count; ix++)
        {
            new(__builtin_addressof(tmp[ix])) T(allocator);     // Requires cotr of T(KAllocator&)
        }
        return tmp;
    }
    static void Construct(T& t, KAllocator& allocator)
    {
        new(__builtin_addressof(t)) T(allocator);
    }
    static T Construct(KAllocator& allocator)
    {
        return T(allocator);
    }
    static void DestructArray(T* p, ULONG count)
    {
        // In-place DTOR for T instances having an allocator
        for (ULONG ix = 0; ix < count; ix++)
        {
            p[ix].~T();
        }

        _deleteArray((UCHAR*)p);
    }
};

template <class T>
class KArray : public KObject<KArray<T>>
{
    KAllocator_Required();         // Enable nesting: KArray<KArray<KArray<KWString>>>  myStringArrayArrayArray(Allocator);

public:
    KArray() = delete;

    //  Constructor
    //
    //  Parameters:
    //    ReserveSize           The initial number of elements to reserve (initial internal size of the array)
    //    GrowByPercent         How much to grow the array when it runs out of space
    //                          as percentage of its current size.
    //                          If set to zero, the array will not grow once it is filled to the ReserveSize limit.
    //
    FAILABLE KArray(
        __in KAllocator& Allocator,
        __in ULONG ReserveSize = 16,
        __in ULONG GrowByPercent = 100
        );

    //  Constructor
    //
    //  Parameters:
    //    InitialSize           The initial number of elements to reserve (initial internal size of the array)
    //    ItemCount             The initial number of occupied elements
    //    GrowByPercent         How much to grow the array when it runs out of space
    //                          as percentage of its current size.
    //                          If set to zero, the array will not grow once it is filled to the ReserveSize limit.
    //
    FAILABLE KArray(
        __in KAllocator& Allocator,
        __in ULONG InitialSize,
        __in ULONG ItemCount,
        __in ULONG GrowByPercent
        );

    // Copy constructor
    //
    // This may fail.  Call Status() afterwards.
    //
    FAILABLE KArray(__in const KArray& Src);

    // Move constructor.
    //
    NOFAIL KArray(__in KArray&& Src);

    // Destructor
    //
    ~KArray();

    // Assignment operator
    //
    // May fail.  Call Status() afterwards.
    FAILABLE KArray& operator=(__in const KArray& Src);

    // Move assignment operator.
    //
    NOFAIL KArray& operator=(__in KArray&& Src);

    // Equals operator.  Two KArrays are defined to be equal if their contents are equal.  T must
    // correctly implement == and !=.
    bool operator==(__in KArray const & other) const;
    bool operator!=(__in KArray const & other) const;

    // IsEmpty
    //
    // Return values:
    //     TRUE         If the array is empty.
    //     FALSE        If the array is not empty.
    BOOLEAN IsEmpty() const;

    // Count
    //
    // Return value:
    //     Returns the number of elements currently in the array (not the reserved size)
    //
    ULONG Count() const;

    // Max
    //
    // Return value:
    //     Returns the maximum number of elements the array can currently contain without a reallocation.
    //
    ULONG Max() const;

    // Append
    //
    // Adds a copy of the parameter to the end of the array.   .
    // Equivalent to vector::push_back in STL.
    //
    // Parameters:
    //    Obj      A reference to the object to be added.  The object must be copy constructible/assignable.
    //
    // Return value:
    //    STATUS_SUCCESS        The object was added to the array.
    //    STATUS_INSUFFICIENT_RESOURCES   Insufficient memory was available to insert the item.
    //
    NTSTATUS Append(__in T const & Obj);

    NTSTATUS Append(__in T&& Obj);


    // InsertAt
    //
    // Inserts a copy of the item at the specified location. This is slower than Append,
    // as it must move elements to make room for the insertion.  Inserting at 0
    // makes the element the first element in the array. Inserting at Count is equivalent
    // to Append.  This can be expensive, as the elements have to be shifted to accommodate the new element
    // at the specified index.
    //
    // The object type must support move semantics.
    //
    // Parameters:
    //    Index         The array location where the new element should reside.  This must be <= Count.
    //    Obj           A reference to the object to be copied. The object must support
    //                  copy construction/assignment.
    // Return value:
    //    STATUS_SUCCESS        The object was added to the array.
    //    STATUS_INSUFFICIENT_RESOURCES   Insufficient memory was available to insert the item.
    //    K_STATUS_OUT_OF_BOUNDS - Index > Count()  
    //
    NTSTATUS InsertAt(__in ULONG Index, __in T const & Obj);


    // Remove
    //
    // Removes an element from the array. Invokes any required destructors.
    //
    // The object type must support move semantics.
    //
    // Parameters:
    //  Index           The index of the array element to be removed.
    //
    // Return value:
    //   TRUE on success
    //   FALSE if the Index was invalid
    //
    #if KArrayImpBlittableType
    BOOLEAN RemoveRange(__in ULONG Index, __in ULONG ForCountOf);
    BOOLEAN Remove(__in ULONG Index) { return RemoveRange(Index, 1); }
    #else
    BOOLEAN Remove(__in ULONG Index);
    BOOLEAN RemoveRange(__in ULONG Index, __in ULONG ForCountOf)
    {
        if (Index >= _Count)
        {
            return FALSE;
        }

        ForCountOf = __min(ForCountOf, (_Count - Index));           // Limit ForCountOf to stay in range of [Index, Count)
        while (ForCountOf > 0)
        {
            if (!Remove(Index))
            {
                return FALSE;
            }
            ForCountOf--;
        }

        return TRUE;
    }
    #endif

    // operator[]
    //
    // Allows array-style access to elements.  This is unchecked and will fail
    // if an invalid Index is used or this is invoked on an empty array.
    //
    // Parameters:
    //   Index          The 0-origin index of the array element to be accessed
    //
    // Return value:
    //   T&             A reference to the element, allowing it to be either read or written.
    T& operator[](__in ULONG Index) const;

    // Clear
    //
    // Make it so that the array has no elements.
    //
    VOID Clear();

    // Data
    //
    // Give access to the raw pointer
    //
    // Return value:
    //   T*             A pointer to the underlying array
    T * Data() const;

    // SetCount
    //
    // Shrink the array or grow the array to a certain size
    //
    // Parameters:
    //   Size           Desired size of the array
    //
    // Return value:
    //    TRUE                  Count was successfully set
    //    FALSE                 Count was higher than the array capacity
    BOOLEAN SetCount(__in ULONG Count);

    // Reserve
    //
    // Ensure the array has the required minimal capacity. It may need to grow the array which can fail.
    // This method does not change the return value of Count().
    //
    // Parameters:
    //   MinimalCapacity    Desired minimal capacity.
    //
    // Return value:
    //    STATUS_SUCCESS                    Capacity is reserved.
    //    STATUS_INSUFFICIENT_RESOURCES     Insufficient memory was available to reserve the capacity.
    NTSTATUS Reserve(__in ULONG MinimalCapacity);
	
	//* basic range-for support
	//
	__forceinline T* begin() const	{ return _Array; }
	__forceinline T* end()   const	{ return _Array + _Count; }

	//* extended enumerator support for range-for
	struct Enumerator
	{
	private:
		KArray<T>&	_array;
		LONG		_begin;
		LONG		_end;
		LONG		_incBy;

	public:
		struct Cursor
		{
		private:
			KArray<T>&	_array;
			LONG		_current;
			LONG		_incBy;

		private:
			friend Enumerator;
			__forceinline Cursor(__in KArray<T>& Array, __in LONG Pos, __in LONG IncBy)	: _array(Array), _current(Pos), _incBy(IncBy) {}

		public:
			__forceinline T& operator*() { return _array[_current]; }					// get current
			__forceinline Cursor& operator++() { _current += _incBy; return *this; }	// next
			__forceinline bool operator!=(__in const Cursor& End) const					// While?
			{ 
				if (_incBy > 0)
				{
					// Going fwd - go until at or past End
					return _current < End._current;
				}
				KAssert(_incBy < 0);

				// Going backwards - go until below or at End
				return ((_current > End._current) && (_current < LONG(_array._Count)));
			}
		};

		__forceinline Cursor begin() const { return Cursor(_array, _begin, _incBy); }
		__forceinline Cursor end() const { return Cursor(_array, _end, 0); }

	private:
		friend KArray;
		__forceinline Enumerator(__in KArray& Array, __in LONG Begin, __in LONG End, __in LONG IncBy) 
			: _array(Array), _begin(Begin), _end(End), _incBy(IncBy) {}
	};

	//* for-each friendly Enumerator - range = [Begin, End)
	__forceinline Enumerator GetEnumerator(__in_opt LONG Begin = 0, __in_opt LONG End = MAXLONG, __in_opt LONG IncBy = 1)
	{
		if (End == MAXLONG)
		{
			End = _Count;
		}

		// Validate params
		KInvariant(IncBy != 0);
		KInvariant((Begin <= (LONGLONG)_Count) && (Begin >= 0));
		KInvariant((End <= (LONGLONG)_Count) && (End >= -1));
		KInvariant(((IncBy < 0) && (Begin >= End)) || ((IncBy > 0) && (End >= Begin)));
		return Enumerator(*this, Begin, End, IncBy);
	}

private:
	friend struct Enumerator;
	friend struct Enumerator::Cursor;

	KAllocator& _Allocator;
    T*          _Array;
    ULONG       _Count;            // How many elements are in use
    ULONG       _Size;             // How many elements will fit without a realloc via Grow()
    ULONG       _GrowByPercent;    // How much to grow by

    // Internal private functions
    NTSTATUS  TestGrow();
    void      Zero();
    NTSTATUS  Initialize();
    void      Cleanup();
    NTSTATUS  CloneFrom(__in const KArray& Src);
};

#ifndef KTL_CORE_LIB
template<typename T>
using KSharedArray = KSharedType<KArray<T>>;
#endif

template <class T> inline
void KArray<T>::Zero()
{
    _Array = nullptr;
    _Count = 0;
    _Size  = 0;
    _GrowByPercent = 0;
}

template <class T> inline
NTSTATUS KArray<T>::Initialize()
{
    if (_Size > 0)
    {
        _Array = KArrayFactory<T>::ConstructArray(_Size, _Allocator);
        if (!_Array)
        {
            _Size = 0;
            _Count = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return STATUS_SUCCESS;
}

#if KArrayImpBlittableType
template <class T> inline
void KArray<T>::Cleanup()
{
    if (!isBlittable<T>::value)
    {
        KArrayFactory<T>::DestructArray(_Array, _Size);
    }

    if (isBlittable<T>::value)
    {
        // In-place DTOR for T instances for array with blittable elements
        for (ULONG ix = 0; ix < _Size; ix++)
        {
            _Array[ix].~T();
        }
        _deleteArray((UCHAR*)_Array);
    }
}

#else
template <class T> inline
void KArray<T>::Cleanup()
{
    KArrayFactory<T>::DestructArray(_Array, _Size);
}
#endif


template <class T> inline
NTSTATUS KArray<T>::CloneFrom(
    __in const KArray& Src
    )
{
    Cleanup();
    Zero();

    _Size = Src._Size;
    _Count = Src._Count;
    _GrowByPercent = Src._GrowByPercent;

    NTSTATUS status = Initialize();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    for (ULONG ix = 0; ix < Src._Count; ix++)
    {
        _Array[ix] = Src._Array[ix];
    }

    return  KObject<KArray<T>>::Status();
}


template <class T> inline
KArray<T>::KArray(
    __in KAllocator& Allocator,
    __in ULONG ReserveSize,
    __in ULONG GrowByPercent
    )
    :   _Allocator(Allocator)
{
    Zero();
    _Size = ReserveSize;
    _GrowByPercent = GrowByPercent;
    KObject<KArray<T>>::SetConstructorStatus(Initialize());
}


template <class T> inline
KArray<T>::KArray(
    __in KAllocator& Allocator,
    __in ULONG InitialSize,
    __in ULONG ItemCount,
    __in ULONG GrowByPercent
    )
    :   _Allocator(Allocator)
{
    ASSERT(ItemCount <= InitialSize);

    Zero();
    _Size = InitialSize;
    _Count = ItemCount;
    _GrowByPercent = GrowByPercent;
    this->SetConstructorStatus(Initialize());
}

template <class T> inline
KArray<T>::KArray(
    __in const KArray& Src
    )
    :   _Allocator(Src._Allocator)
{
    Zero();
    CloneFrom(Src);
}

template <class T> inline
KArray<T>::KArray(
    __in KArray&& Src
    )
    :   _Allocator(Src._Allocator)
{
    _Array = Src._Array;
    _Size = Src._Size;
    _Count = Src._Count;
    _GrowByPercent = Src._GrowByPercent;
    this->SetConstructorStatus(Src.Status());

    Src.Zero();
}

template <class T> inline
KArray<T>::~KArray(
    )
{
    Cleanup();
}

template <class T> inline
KArray<T>& KArray<T>::operator =(
    __in const KArray& Src
    )
{
    if (&Src == this)
    {
        return *this;
    }

    Cleanup();
    Zero();
    KObject<KArray<T>>::SetStatus(CloneFrom(Src));
    return *this;
}

template <class T> inline
KArray<T>& KArray<T>::operator =(
    __in KArray&& Src
    )
{
    if (&Src == this)
    {
        return *this;
    }

    Cleanup();
    Zero();

    KObject<KArray<T>>::SetStatus(Src.Status());
    _Allocator = Src._Allocator;
    _Array = Src._Array;
    _Size = Src._Size;
    _Count = Src._Count;
    _GrowByPercent = Src._GrowByPercent;

    Src.Zero();

    return *this;
}

template <class T> inline
bool KArray<T>::operator ==(
    __in KArray const & other
    ) const
{
    if (&other == this)
    {
        return TRUE;
    }

    if (_Count != other.Count())
    {
        return FALSE;
    }

    for (ULONG i = 0; i < _Count; i++)
    {
        if ((*this)[i] != other[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

template <class T> inline
bool KArray<T>::operator !=(
    __in KArray const & other
    ) const
{
    return !(*this == other);
}

template <class T> inline
BOOLEAN KArray<T>::IsEmpty() const
{
    return _Count ? FALSE : TRUE;
}

template <class T> inline
ULONG KArray<T>::Count() const
{
    return _Count;
}

template <class T> inline
ULONG KArray<T>::Max() const
{
    return _Size;
}

template <class T> inline
NTSTATUS KArray<T>::Append(
    __in T const & Object
    )
{
    if (!NT_SUCCESS(TestGrow()))
    {
        return KObject<KArray<T>>::Status();
    }

    _Array[_Count++] = Object;
    return STATUS_SUCCESS;
}

template <class T> inline
NTSTATUS KArray<T>::Append(
    __in T&& Object
    )
{
    if (!NT_SUCCESS(TestGrow()))
    {
        return  KObject<KArray<T>>::Status();
    }

    _Array[_Count++] = Ktl::Move(Object);
    return STATUS_SUCCESS;
}


#if KArrayImpBlittableType
template <class T> inline
NTSTATUS KArray<T>::InsertAt(
    __in ULONG Index,
    __in T const & Obj
)
{
    constexpr bool isBlittableType = isBlittable<T>::value;
    constexpr bool  isBlittableTypeDefaultIsZero = isBlittableDefaultIsZero<T>::value;
    static_assert((
        (isBlittableTypeDefaultIsZero && isBlittableType)    ||
        (!isBlittableTypeDefaultIsZero && isBlittableType)   ||
        (!isBlittableTypeDefaultIsZero && !isBlittableType)),
        "KIsBlittableType_DefaultIsZero()/KIs_BlittableType() misconfigured for type T");

    const bool      useElementByElementCopy = !(isBlittableType && isBlittableTypeDefaultIsZero);

    if (Index > _Count)
    {
        return K_STATUS_OUT_OF_BOUNDS;
    }

    if (!NT_SUCCESS(TestGrow()))
    {
        return this->Status();
    }

    if (useElementByElementCopy)
    {
        ULONG i;

        for (i =_Count; i > Index; i--)
        {
            // TODO: Why is this not using move semantics - kept the same for back compat in just in case 
            _Array[i] = _Array[i - 1];
        }

        KInvariant(i == Index);
        _Array[i] = Obj;
    }
    else
    {
        if (Index < _Count)
        {
            // Reverse move elements in the range of [_Index, _Count) to [_Index + 1, _Count]
            RtlMoveMemory(__builtin_addressof(_Array[Index + 1]), __builtin_addressof(_Array[Index]), (_Count - Index) * sizeof(T));

            // re-init the moved element @ Index 
            RtlZeroMemory(__builtin_addressof(_Array[Index]), sizeof(T));
            _Array[Index].T::~T();
            new(__builtin_addressof(_Array[Index])) T(Obj);
        }
        else
        {
            _Array[Index] = Obj;
        }
    }
    _Count++;
    return STATUS_SUCCESS;
}

#else
template <class T> inline
NTSTATUS KArray<T>::InsertAt(
    __in ULONG Index,
    __in T const & Obj
)
{
    if (!NT_SUCCESS(TestGrow()))
    {
        return this->Status();
    }

    ULONG i = _Count;
    for (; i > Index; i--)
    {
        _Array[i] = _Array[i - 1];
    }

    ASSERT(i == Index);

    _Array[i] = Obj;
    _Count++;
    return STATUS_SUCCESS;
}
#endif


#if KArrayImpBlittableType
template <class T> inline
NTSTATUS KArray<T>::TestGrow()
{
    if (_Count == _Size)
    {
#if !defined(PLATFORM_UNIX)
        constexpr bool isBlittableType = isBlittable<T>::value;
        constexpr bool  isBlittableTypeDefaultIsZero = isBlittableDefaultIsZero<T>::value;
        static_assert((
            (isBlittableTypeDefaultIsZero && isBlittableType) ||
            (!isBlittableTypeDefaultIsZero && isBlittableType) ||
            (!isBlittableTypeDefaultIsZero && !isBlittableType)),
            "KIsBlittableType_DefaultIsZero()/KIs_BlittableType() misconfigured for type T");
#endif

        if (_GrowByPercent == 0)
        {
            // Disallow expansion was requested
            KObject<KArray<T>>::SetStatus(STATUS_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        HRESULT hr;
        ULONG NewSize;
        hr = ULongMult(_Size, _GrowByPercent, &NewSize);
        KInvariant(SUCCEEDED(hr));
        NewSize = __max(1, ((NewSize) / 100));
        hr = ULongAdd(_Size, NewSize, &NewSize);
        KInvariant(SUCCEEDED(hr));

        T* tmp = nullptr;

        if (!isBlittable<T>::value)
        {
            tmp = KArrayFactory<T>::ConstructArray(NewSize, _Allocator);
            if (tmp == nullptr)
            {
                KObject<KArray<T>>::SetStatus(STATUS_INSUFFICIENT_RESOURCES);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            for (ULONG i = 0; i < _Count; i++)
            {
                tmp[i] = Ktl::Move(_Array[i]);
            }

            Cleanup();
        }

        if (isBlittable<T>::value)
        {
            ULONG result;
            hr = ULongMult(NewSize, sizeof(T), &result);
            KInvariant(SUCCEEDED(hr));
            tmp = (T*)_newArray<UCHAR>(KTL_TAG_ARRAY, _Allocator, result);
            if (tmp == nullptr)
            {
                KObject<KArray<T>>::SetStatus(STATUS_INSUFFICIENT_RESOURCES);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            // Blit all existing elements into new storage
			KMemCpySafe(tmp, result, _Array, _Size * sizeof(T));

            // Then ctor the rest through NewSize
            // Must in-place ctor new objects beyond existing _Size
            for (ULONG ix = _Size; ix < NewSize; ix++)
            {
                KArrayFactory<T>::Construct(tmp[ix], _Allocator);
            }

            _deleteArray((UCHAR*)_Array);
            _Array = nullptr;
        }

        _Array = tmp;
        _Size = NewSize;
    }

    return KObject<KArray<T>>::Status();
}

#else
template <class T> inline
NTSTATUS KArray<T>::TestGrow()
{
    if (_Count == _Size)
    {
        if (_GrowByPercent == 0)
        {
            // Disallow expansion was requested
            KObject<KArray<T>>::SetStatus(STATUS_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        HRESULT hr;
        ULONG NewSize;
        hr = ULongMult(_Size, _GrowByPercent, &NewSize);
        KInvariant(SUCCEEDED(hr));
        NewSize = __max(1, ((NewSize) / 100));
        hr = ULongAdd(_Size, NewSize, &NewSize);
        KInvariant(SUCCEEDED(hr));

        T* tmp = KArrayFactory<T>::ConstructArray(NewSize, _Allocator);
        if (tmp == nullptr)
        {
            KObject<KArray<T>>::SetStatus(STATUS_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for (ULONG i = 0; i < _Count; i++)
        {
            tmp[i] = Ktl::Move(_Array[i]);
        }

        Cleanup();
        _Array = tmp;
        _Size = NewSize;
    }

    return KObject<KArray<T>>::Status();

#endif

#if KArrayImpBlittableType
template <class T> inline
BOOLEAN KArray<T>::RemoveRange(__in ULONG Index, __in ULONG ForCountOf)
{
    if (Index >= _Count)
    {
        return FALSE;
    }

    ForCountOf = __min(ForCountOf, (_Count - Index));           // Limit ForCountOf to stay in range of [Index, Count)
    if (ForCountOf > 0)
    {
        if (!isBlittable<T>::value)
        {
            for (ULONG ix = Index; (ix + ForCountOf) < _Count; ix++)
            {
                _Array[ix] = Ktl::Move(_Array[ix + ForCountOf]);
            }

            // In-place dtor/ctor any elements in the interval [_Count - ForCountOf, _Count)
            for (ULONG ix = (_Count - ForCountOf); ix < _Count; ix++)
            {
                _Array[ix].~T();
                KArrayFactory<T>::Construct(_Array[ix], _Allocator);
            }
        }

        if (isBlittable<T>::value)
        {
            // Fast blittable

            // Dtor any elements in the range of [Index, Index + ForCountOf)
            for (ULONG ix = Index; ix < (Index + ForCountOf); ix++)
            {
                _Array[ix].T::~T();
            }

            if ((Index + ForCountOf) < _Count)
            {
                // Blit all elements in the range of [Index + ForCountOf, Count) to [Index, Count - ForCountOf]
                // __movsb should be safe for this class of overlapped copy
                __movsb(reinterpret_cast<PUCHAR>(__builtin_addressof(_Array[Index])), 
                    reinterpret_cast<PUCHAR>(__builtin_addressof(_Array[Index + ForCountOf])), (_Count - Index - ForCountOf) * sizeof(T));
            }

            // In-place ctor any elements in the interval [_Count - ForCountOf, _Count)
            for (ULONG ix = (_Count - ForCountOf); ix < _Count; ix++)
            {
                KArrayFactory<T>::Construct(_Array[ix], _Allocator);
            }
        }

        _Count -= ForCountOf;
    }

    return TRUE;
}

#else
template <class T> inline
BOOLEAN KArray<T>::Remove(
    __in ULONG Index
)
{
    if (Index >= _Count)
    {
        return FALSE;
    }

    ULONG ix = Index;
    for (; ix < _Count - 1; ix++)
    {
        _Array[ix] = Ktl::Move(_Array[ix + 1]);
    }

    _Array[ix] = KArrayFactory<T>::Construct(_Allocator);

    _Count--;
    return TRUE;
}
#endif

template <class T> inline
T& KArray<T>::operator[](
    __in ULONG Index
    ) const
{
    KInvariant(Index < _Count);         // OUT OF RANGE
    return _Array[Index];
}

template <class T> inline
VOID KArray<T>::Clear(
    )
{
    for (ULONG ix = 0; ix < _Count; ix++)
    {
        // In-place DTOR/CTOR T instances
        _Array[ix].~T();
        KArrayFactory<T>::Construct(_Array[ix], _Allocator);
    }
    _Count = 0;
}

template <class T> inline
T * KArray<T>::Data(
    ) const

{
    return _Array;
}

template <class T> inline
BOOLEAN KArray<T>::SetCount(
    ULONG Count)
{
    if (Count <= _Size)
    {
        ULONG oldSize = _Count;
        _Count = Count;

        for (ULONG ix = _Count; ix < oldSize; ix++)
        {
            _Array[ix] = KArrayFactory<T>::Construct(_Allocator);
        }

        return TRUE;
    }

    return FALSE;
}

template <class T> inline
NTSTATUS KArray<T>::Reserve(
    __in ULONG MinimalCapacity
    )
{
    if (MinimalCapacity <= _Size)
    {
        return STATUS_SUCCESS;
    }

    T* tmp = KArrayFactory<T>::ConstructArray(MinimalCapacity, _Allocator);
    if (tmp == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (ULONG i = 0; i < _Count; i++)
    {
        tmp[i] = Ktl::Move(_Array[i]);
    }
    KArrayFactory<T>::DestructArray(_Array, _Size);

    _Array = tmp;
    _Size = MinimalCapacity;
    return STATUS_SUCCESS;
}


