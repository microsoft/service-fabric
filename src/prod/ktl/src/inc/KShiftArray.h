/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    KShiftArray.h

Abstract:

    This header file defines the KShiftArray container class.
    A shift array is a fixed-size sliding window over a larger, possibly infinite array.

    A user can append new elements into the shift array. If the shift array is full,
    the oldest elements will be overwritten to make room for the new elements.

    A user can read back up to the last N elements in the larger array,
    where N is the fixed size of the shift array.

Author:

    Peng Li (pengli)    22-Feb-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#pragma once

template <class T, ULONG MaxNumberOfElements>
class KShiftArray : public KObject<KShiftArray<T, MaxNumberOfElements>>
{
public:

    //  Constructor
    //
    //  Parameters:
    //
    //      Allocator - Supplies the allocator object.
    //
    //      AllocationTag - Supplies the allocation tag.
    //
    FAILABLE KShiftArray(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_SHIFT_ARRAY
        );

    // Destuctor
    //
    ~KShiftArray();

    // IsEmpty
    //
    // Return values:
    //
    //     TRUE         If the array is empty.
    //
    //     FALSE        If the array is not empty.
    //
    BOOLEAN IsEmpty();

    // Count
    //
    // Return value:
    //
    //      Returns the number of elements currently in the array (not the max size)
    //      The returned value is in [0, MaxNumberOfElements].
    //
    ULONG Count();

    // Max
    //
    // Return value:
    //
    //      Returns the maximum number of elements the array can contain.
    //
    ULONG Max();

    // Append
    //
    // Adds a copy of the parameter to the end of the array.
    // Because the array has a fixed size, an append may overwrite the oldest element,
    // but will never fail.
    //
    // Parameters:
    //
    //      Obj - Supplies a reference to the object to be added.  The object must be copy constructible/assignable.
    //
    // Return value:
    //
    //      None
    //
    VOID Append(
        __in T& Obj
        );

    VOID Append(
        __inout T&& Obj
        );

    // operator[]
    //
    // Allows array-style access to elements.  This is unchecked and will fail
    // if an invalid Index is used or this is invoked on an empty array.
    //
    // If Index equals to Count() - 1, this routine returns the last element.
    // Essentially the last Count() elements are visible to the user and can be accessed
    // using this operator overload.
    //
    // Parameters:
    //   Index          The 0-origina index of the array element to be accessed
    //
    // Return value:
    //   T&             A referece to the element, allowing it to be either read or written.
    T& operator[](
        __in ULONG Index
        );

    // Clear
    //
    // Make it so that the array has no elements.
    //
    VOID Clear();

private:

    //
    // A fixed-size array of elements. The array size is MaxNumberOfElements.
    //
    T* _Array;

    //
    // This is the logical index of the slot to be used for the next Append().
    // It is the number of times an element is appended into the array.
    // To map it to a physical slot, use (_NextLogicalArrayIndex % MaxNumberOfElements).
    //
    ULONGLONG _NextLogicalArrayIndex;

    //
    // The number of valid elements in the array. This value is in [0, MaxNumberOfElements].
    //
    ULONG _Count;
};

template <class T, ULONG MaxNumberOfElements> inline
KShiftArray<T, MaxNumberOfElements>::KShiftArray(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    _Array = _newArray<T>(AllocationTag, Allocator, MaxNumberOfElements);
    if (!_Array)
    {
        this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _NextLogicalArrayIndex = 0;
    _Count = 0;
}

template <class T, ULONG MaxNumberOfElements> inline
KShiftArray<T, MaxNumberOfElements>::~KShiftArray(
    )
{
    _deleteArray(_Array);
    _Array = nullptr;

    _NextLogicalArrayIndex = 0;
    _Count = 0;
}

template <class T, ULONG MaxNumberOfElements> inline
BOOLEAN KShiftArray<T, MaxNumberOfElements>::IsEmpty()
{
    return _Count ? FALSE : TRUE;
}

template <class T, ULONG MaxNumberOfElements> inline
ULONG KShiftArray<T, MaxNumberOfElements>::Count()
{
    return _Count;
}

template <class T, ULONG MaxNumberOfElements> inline
ULONG KShiftArray<T, MaxNumberOfElements>::Max()
{
    return MaxNumberOfElements;
}

template <class T, ULONG MaxNumberOfElements> inline
VOID KShiftArray<T, MaxNumberOfElements>::Append(
    __in T& Obj
    )
{
    _Array[_NextLogicalArrayIndex % MaxNumberOfElements] = Obj;
    _NextLogicalArrayIndex++;

    _Count = __min(_Count + 1, MaxNumberOfElements);
}

template <class T, ULONG MaxNumberOfElements> inline
VOID KShiftArray<T, MaxNumberOfElements>::Append(
    __inout T&& Obj
    )
{
    _Array[_NextLogicalArrayIndex % MaxNumberOfElements] = Obj;
    _NextLogicalArrayIndex++;

    _Count = __min(_Count + 1, MaxNumberOfElements);
}

template <class T, ULONG MaxNumberOfElements> inline
T& KShiftArray<T, MaxNumberOfElements>::operator[](
    __in ULONG Index
    )
{
    KAssert(Index < _Count);
    KAssert(_NextLogicalArrayIndex >= _Count);

    ULONGLONG logicalIndex = (_NextLogicalArrayIndex - _Count) + Index;
    return _Array[logicalIndex % MaxNumberOfElements];
}

template <class T, ULONG MaxNumberOfElements> inline
VOID KShiftArray<T, MaxNumberOfElements>::Clear()
{
    for (ULONG ix = 0; ix < _Count; ix++)
    {
        _Array[ix] = T();
    }

    _NextLogicalArrayIndex = 0;
    _Count = 0;
}

