/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kqueue

    Description:
      Kernel Tempate Library (KTL): KQueue object.

      Implements a simple, unguarded FIFO queue.

    History:
      raymcc          16-Aug-2010         Initial version.
      raymcc          02-Sep-2010         Separation of implementation; simplifications

--*/

#pragma once


template <class T>
class KQueue : public KObject<KQueue<T>>
{
    KAllocator_Required();

public:
    //
    // Constructor
    //
    // Constructs an empty queue.  Can't fail.
    //
    NOFAIL KQueue(KAllocator& Allocator);

    // Destructor
    //
    // Deallocates any items still in the queue during destruction.
    //
   ~KQueue();

    // Copy constructor
    //
    FAILABLE KQueue(
        __in const KQueue& Src
        );

    // Assignment
    //
    FAILABLE KQueue& operator=(
        __in const KQueue& Src
        );

    // IsEmpty
    //
    // Return value:
    //   TRUE if the queue is empty.
    //   FALSE if the queue is not empty.
    //
    BOOLEAN IsEmpty();

    // Count
    //
    // Returns the number of items in the queue.
    //
    ULONG Count();

    // Peek
    //
    // Returns a reference to the next item (head) in the queue. This is unchecked and will
    // fault if the queue is empty.
    //
    // Return value:
    //   A reference to the next item in the queue.
    //
    T& Peek();


    // PeekTail
    //
    // Returns a reference to the most recently enqueued item (tail).   This is unchecked.
    // Its primary use is to check the status of an enqueued object which might have
    // run into out-of-memory during the copy construction necessary to enqueue it.
    //
    T& PeekTail();

    // Deq
    //
    // Removes and deallocates the next item from the queue (a dequeue operation).
    // This is typically done after executing a Peek() to read the item.
    //
    // Return values:
    //    TRUE if an item was dequeued.
    //    FALSE if the queue was tempty.
    //
    BOOLEAN Deq();

    // Deq
    //
    // Removes and head of the queue and delivers it to the caller's object.
    //
    // Parameters:
    //      Receiver        Receives the object at the front of the queue.  The
    //                    object must support assignment semantics.
    //
    // Return value:
    //    FALSE            If the queue is empty
    //    TRUE            If Receiver received the dequeued object
    //
    BOOLEAN Deq(
        __inout T& Receiver
        );

    // Enq
    //
    // Enqueues the specified item.  The object must support copy construction.
    // Because of a FAILABLE copy constructor, the caller may wish to call PeekTail().Status()
    // to verify that the object was successfully copied during the enqueue.
    //
    // Parameters:
    //    Object      A reference to the object to be copied into the queue.
    //
    // Return value:
    //    TRUE        If the object was successfully added to the queue.
    //    FALSE       Insufficient memory was available to enqueue the item.
    //
    BOOLEAN Enq(
        __in T Object
        );

    // Reset
    //
    // Used for a cheap iteration over the queue without dequeuing anything.
    //
    void
    ResetCursor()
    {
        _Cursor = _Head;
    }

    // Next
    //
    // Used after a reset to get the next element in the queue in a simulated dequeue.
    //
    // Parameter:
    //      Receiver        Receives the next object, if there is one.
    //
    // Return value:
    //      TRUE if an object was delivered, FALSE if end-of-queue.
    //
    BOOLEAN
    Next(
        __out T& Receiver
        )
    {
        if (!_Cursor)
        {
            return FALSE;
        }
        Receiver = _Cursor->_Object;
        _Cursor = _Cursor->_Next;
        return TRUE;
    }

private:
    // Private functions
	KQueue();
    void Zero();
    void Cleanup();
    NTSTATUS CloneFrom(
        __in KQueue& Src
        );

    // Data members
    struct Entry
    {
        Entry* _Next;
        T _Object;
        Entry(T& src) : _Object(src), _Next(nullptr) { }
    };

	KAllocator&	_Allocator;
    Entry*		_Tail;
    Entry*		_Head;
    Entry*		_Cursor;
    ULONG		_Count;
};


template <class T> inline
KQueue<T>::KQueue(KAllocator& Allocator)
	:	_Allocator(Allocator)
{
    Zero();
}

template <class T> inline
KQueue<T>::~KQueue()
{
    Cleanup();
}

template <class T> inline
KQueue<T>::KQueue(
    __in const KQueue& Src
    )
	:	_Allocator(Src._Allocator)
{
    Zero();
    CloneFrom(Src);
}

template <class T> inline
KQueue<typename T>& KQueue<T>::operator=(
    __in const KQueue& Src
        )
{
    Cleanup();
    CloneFrom(&Src);
    return *this;
}

template <class T> inline
BOOLEAN KQueue<T>::IsEmpty()
{
    return _Count == 0 ? TRUE : FALSE;
}

template <class T> inline
ULONG KQueue<T>::Count()
{
   return _Count;
}

template <class T> inline
T& KQueue<T>::Peek()
{
    return _Head->_Object;
}

template <class T> inline
T& KQueue<T>::PeekTail()
{
    return _Tail->_Object;
}


template <class T> inline
BOOLEAN KQueue<T>::Deq()
{
    if (!_Count)
    {
        return FALSE;
    }
    Entry* Tmp = _Head;
    _Head = _Head->_Next;
    _delete(Tmp);
    _Count--;
    if (_Head == nullptr)
    {
        _Tail = nullptr;
    }
    return TRUE;
}

template <class T> inline
BOOLEAN KQueue<T>::Deq(
    __inout T& Receiver
    )
{
    if (!IsEmpty())
    {
       Receiver = Peek();
       Deq();
       return TRUE;
    }
    return FALSE;
}

template <class T> inline
BOOLEAN KQueue<T>::Enq(
    __in T Object
    )
{
    if (!NT_SUCCESS(this->Status()))
    {
        return FALSE;
    }
    Entry* pNew = _new(KTL_TAG_QUEUE, _Allocator) Entry(Object);
    if (!pNew)
    {
        this->SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }
    if (_Tail)
    {
        _Tail->_Next = pNew;
    }
    _Tail = pNew;
    if (!_Head)
    {
        _Head = _Tail;
    }
    _Count++;
    return TRUE;
}

template <class T> inline
void KQueue<T>::Zero()
{
    _Count = 0;
    KObject<KQueue<T>>::SetStatus(STATUS_SUCCESS);
    _Cursor = _Head = _Tail = nullptr;
}

template <class T> inline
void KQueue<T>::Cleanup()
{
    while (_Count)
    {
        Deq();
    }
 }

