/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kstack

    Description:
      Kernel Tempate Library (KTL): KStack object.
      Implements a simple unguarded LIFO stack

    History:
      raymcc          16-Aug-2010         Initial version.
      raymcc          02-Sep-2010         Separation of implementation; simplifications

--*/

#pragma once


template <class T>
class KStack : public KObject<KStack<T>>
{
public:
    // Constructor
    //
    // Constructs an empty stack.  Doesn't fail.
    //
    NOFAIL KStack(KAllocator& Allocator);

    // Copy constructor
    //
    FAILABLE KStack(
        __in const KStack& Src
        );

    // Assignment operator
    //
    FAILABLE KStack& operator=(
        __in KStack& Src
        );

    // Destructor
    //
    // Deallocates all elements still on the stack at destruct-time.
    //
    ~KStack();

    //  IsEmpty
    //
    //  Return value:
    //    TRUE if the stack is empty.
    //    FALSE if the stack is not empty.
    //
    BOOLEAN IsEmpty();

    // Count
    //
    // Return value:
    //   The number of elements on the stack
    //
    ULONG Count();

    // Peek
    //
    // Return value:
    //   Returns a reference to the top element of the stack.  This is
    //   unchecked (for speed) and will fault on an empty stack.
    //
    T& Peek();

	// Push
	//
	// Pushes a value onto the stack.  The object is assumed to have
	// no fail properties, be assignable and copy-constructible.
	// This method pushes by copy and is suitable for simple types.
    // Because the object is copied several times before it is added to the list,
	// the assignment and copy constructor for the type should have NOFAIL
	// properties.
    //
    // If the object has FAILABLE copy constructors, then a Peek() should
    // be performed after the push to check the Status() method of the object
    // that was pushed, i.e.,  NTSTATUS Check = Peek().Status();
	//
    // To avoid copies, consider instantiating the stack as Stack<MyType&>
    // or Stack<MyType*> or a stack of KSharedPtr types which will avoid
    // copy construction of the underlying object.
    //
	// Parameters:
    //    Object        A copy of the value to be pushed.
    //
    // Return value:
    //    STATUS_SUCCESS    If the object was successfully stacked.
    //    STATUS_INSUFFICIENT_RESOURCES if a new internal link could not be allocated.
    //
	BOOLEAN Push(
		__in T Object
		);


    // Pop
    //
    // Pops and destructs the top element of the stack.
    // This is unchecked and should not be called on an empty stack.
    //
    // Peek() is called to read the top element, followed by a Pop
    // to discard it.
    //
    void Pop();

    // Pop
    //
    // This overload returns and removes the item at the top of the stack.
    //
    // Parameters:
    //  Destination     Receives the element at the top of the stack.
    //
    // Return value:
    //  TRUE if the element was delivered and popped
    //  FALSE if the stack was empty
    //
    BOOLEAN Pop(
        __out T& Destination
        );

private:

    void Zero();
    void Cleanup();
    NTSTATUS CloneFrom(KStack& Src);

    struct Entry
    {
        Entry* _Next;
        T _Object;
        Entry(T& Src) : _Object(Src), _Next(nullptr) {}
    };

	KAllocator& _Allocator;
    Entry* _Top;
    ULONG  _Count;
};

//
// Implementation
//
template <class T> inline
KStack<T>::KStack(KAllocator& Allocator)
	:	_Allocator(Allocator)
{
    _Top = nullptr;
    _Count = 0;
    this->SetStatus(STATUS_SUCCESS);
}

template <class T> inline
KStack<T>::~KStack()
{
    while (_Count)
    {
		Pop();
    }
}

template <class T> inline
BOOLEAN KStack<T>::IsEmpty()
{
    return _Count == 0 ? TRUE : FALSE;
}


template <class T> inline
ULONG KStack<T>::Count()
{
    return _Count;
}


template <class T> inline
T& KStack<T>::Peek()
{
    return _Top->_Object;
}

template <class T> inline
BOOLEAN KStack<T>::Push(
    __in T Object
    )
{
    if (!NT_SUCCESS(this->Status()))
    {
        return FALSE;
    }
    Entry* pNew = _new(KTL_TAG_STACK, _Allocator) Entry(Object);
    if (!pNew)
    {
        this->SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }
    pNew->_Next = _Top;
    _Top = pNew;
    _Count++;
    return TRUE;
}

template <class T> inline
void KStack<T>::Pop()
{
    Entry* Tmp = _Top;
    _Top = _Top->_Next;
    _delete(Tmp);
    _Count--;
}


template <class T> inline
BOOLEAN KStack<T>::Pop(
    __out T& Destination
    )
{
    if (!IsEmpty())
    {
        Destination = Peek();
        Pop();
        return TRUE;
    }
    return FALSE;
}
