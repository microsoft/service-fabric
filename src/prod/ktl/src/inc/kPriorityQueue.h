/*++
(c) 2010 by Microsoft Corp. All Rights Reserved.

Description:
    Kernel Tempate Library (KTL): KPriorityQueue.

    Implements a none thread-safe priority queue using binary heap (with an array as backing container).
    Provides O(1) Peek and O(log n) Pop & Push.

    Note that priorities of the items are implicit: They are determined using less-than operator.

    In future we can add support for using Binomial or Fibonacci heap instead of binary heap as the underlying abstract data type.
    By doing so we could reduce the Push complexity to O (1).
    Binary heap was chosen as the first ADT due to its low memory footprint, compactness and predictable accesses pattern.

User Documentation
    KPriorityQueue is an abstract data type where highest priority item is the first item that will be returned.
    Highest priority item is the one with the lowest value.
    Less-than operator or ComparisonFunction is used to determine order.

    For example, if [1, 3, 2, 1] are pushed in to a KPriorityQueue<int> in order, 
    KPriorityQueue will pop them in the following order: [1, 1, 2, 3]

    Common use case of KPriorityQueue is for n-way merge of sorted lists.

    Note that if T can possibly have a large memory footprint, 
    than you should use a KPriorityQueue<KSharedPtr<T>> instead of KPriorityQueue<T>.

    Example Usage
        NTSTATUS status = KPriorityQueue<int>::Create(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), kPriorityQueueSPtr);
        if(!NT_SUCCESS(status)) return;

        kPriorityQueueSPtr->IsEmpty();                      // TRUE

        BOOLEAN pushSuccess;
        pushSuccess = kPriorityQueueSPtr->Push(1);          // TRUE
        pushSuccess = kPriorityQueueSPtr->Push(3);          // TRUE
        pushSuccess = kPriorityQueueSPtr->Push(2);          // TRUE
        pushSuccess = kPriorityQueueSPtr->Push(1);          // TRUE

        kPriorityQueueSPtr->IsEmpty();                      // FALSE

        BOOLEAN popStatus
        BOOLEAN peekSuccess
        int item;
        popStatus = priorityQueueSPtr->Peek(item);          // TRUE, item = 1
        peekSuccess = priorityQueueSPtr->Pop(item);         // TRUE, item = 1

        peekSuccess = priorityQueueSPtr->Peek(item);        // TRUE, item = 1
        popStatus = priorityQueueSPtr->Pop(item);           // TRUE, item = 1

        peekSuccess = priorityQueueSPtr->Peek(item);        // TRUE, item = 2
        popStatus = priorityQueueSPtr->Pop(item);           // TRUE, item = 2

        peekSuccess = priorityQueueSPtr->Peek(item);        // TRUE, item = 3
        popStatus = priorityQueueSPtr->Pop(item);           // TRUE, item = 3

        peekSuccess = priorityQueueSPtr->Peek(item);        // FALSE
        popStatus = priorityQueueSPtr->Pop(item);           // FALSE

        kPriorityQueueSPtr->IsEmpty();                      // TRUE
--*/

#pragma once

template <class T>
class KPriorityQueue : public KObject<KPriorityQueue<T>>
{
    K_DENY_COPY(KPriorityQueue);
    
public:
    typedef KDelegate<int(__in T const & KeyOne, __in T const & KeyTwo)> ComparisonFunctionType;

public:
    // Constructor
    //
    // Constructs an empty priority queue. Can fail.
    //
    FAILABLE KPriorityQueue(__in KAllocator& Allocator);

    // Constructor
    //
    // Constructs an empty priority queue with comparison function. Can fail. 
    //
    FAILABLE KPriorityQueue(__in KAllocator& Allocator, __in ComparisonFunctionType comparisonFunction);

    // Destructor
    //
    ~KPriorityQueue();

    // Checks whether the priority queue is empty.
    //
    // Return value:
    //   TRUE                               - If empty.
    //   FALSE                              - If not empty.
    //
    BOOLEAN IsEmpty() const;

    // Peeks at the head of the priority queue (min item) without removing it.
    //
    // Out values:
    //   Item           - Smallest item (highest priority) in the priority queue if operation was successful.
    // Return value:
    //   TRUE           - If the operation was successful.
    //   FALSE          - If the operation failed.
    //
    BOOLEAN Peek(__out T& Item) const;

    // Inserts an item into the priority queue.
    //
    // In Paramenters:
    //  Item            - Item to be pushed.
    //
    // Return value:
    //  STATUS_SUCCESS  - If the operation was successful.
    //  !               - If the operation failed.
    NTSTATUS Push(__in T const & Item);

    // Removes the head of the priority queue (min item) and returns it.
    //
    // Out Parameters:
    //   Item           - Smallest item (highest priority) in the priority queue if operation was successful.
    //
    // Return value:
    //   TRUE           - If the operation was successful.
    //   FALSE          - If the operation failed.
    //
    BOOLEAN Pop(__out T& Item);

private:
    // Functions for min-heapifing specific sub-tree.
    void BubbleUp(__in ULONG Index);
    void BubbleDown(__in ULONG Index);

    // Index queries for binary tree implementation using an array.
    __forceinline ULONG GetLeftChildIndex(__in ULONG Index) const;
    __forceinline ULONG GetRightChildIndex(__in ULONG Index) const;
    __forceinline ULONG GetParentIndex(__in ULONG Index) const;
    __forceinline ULONG GetTailIndex() const;

    __forceinline void Swap(__in T& a, __in T& b);

    __forceinline bool IsLessThan(__in T const & a, __in T const & b);

private:
    // Array backing the binary heap.
    KArray<T>       _KArray;

    // Comparison function that can be used for items that do not implement operator <.
    ComparisonFunctionType  _ComparisonFunction;
};

template <class T> inline
KPriorityQueue<T>::KPriorityQueue(__in KAllocator& Allocator)
    : _KArray(KArray<T>(Allocator))
    , _ComparisonFunction()
{
    this->SetConstructorStatus(_KArray.Status());
}

template <class T> inline
KPriorityQueue<T>::KPriorityQueue(
    __in KAllocator& Allocator,
    __in ComparisonFunctionType comparisonFunction)
    : _KArray(KArray<T>(Allocator))
    , _ComparisonFunction(comparisonFunction)
{
    this->SetConstructorStatus(_KArray.Status());
}

template <class T> inline
KPriorityQueue<T>::~KPriorityQueue()
{
}

template <class T>
ULONG KPriorityQueue<T>::GetLeftChildIndex(__in ULONG Index) const
{
    KAssert(Index < _KArray.Count());

    return 2 * Index;
}

template <class T>
ULONG KPriorityQueue<T>::GetRightChildIndex(__in ULONG Index) const
{
    KAssert(Index < _KArray.Count());

    return (2 * Index) + 1;
}

template <class T>
ULONG KPriorityQueue<T>::GetParentIndex(__in ULONG Index) const
{
    KAssert(Index < _KArray.Count());

    return Index / 2;
}

template <class T>
ULONG KPriorityQueue<T>::GetTailIndex() const
{
    return _KArray.Count() - 1;
}

template <class T> inline
void KPriorityQueue<T>::Swap(__in T& A, __in T& B)
{
    T temp = Ktl::Move(A);
    A = Ktl::Move(B);
    B = Ktl::Move(temp);
}

template <class T>
bool KPriorityQueue<T>::IsLessThan(__in T const & a, __in T const & b)
{
    if (!_ComparisonFunction)
    {
        return a < b;
    }

    return _ComparisonFunction(a, b) == -1;
}

template <class T> inline
BOOLEAN KPriorityQueue<T>::IsEmpty() const
{
    return _KArray.Count() == 0 ? TRUE : FALSE;
}

template <class T> inline
BOOLEAN KPriorityQueue<T>::Peek(__out T& Value) const
{
    if (_KArray.Count() == 0)
    {
        return FALSE;
    }

    Value = _KArray[0];

    return TRUE;
}

template <class T> inline
NTSTATUS KPriorityQueue<T>::Push(__in T const & Value)
{
    // Cannot insert more than max capacity.
    if(_KArray.Count() == INT_MAX)
    {
        return STATUS_NOT_FOUND;
    }

    // Code assumes that Append is atomic (either fails or is successful - no side effect).
    NTSTATUS status = _KArray.Append(Value);
    if(!NT_SUCCESS(status))
    {
        return status;
    }

    BubbleUp(GetTailIndex());

    return STATUS_SUCCESS;
}

template <class T> inline
BOOLEAN KPriorityQueue<T>::Pop(__out T& Value)
{
    if(_KArray.Count() == 0)
    {
        return FALSE;
    }
    
    if (_KArray.Count() == 1)
    {
        // Set out parameter to head of the binary heap.
        Value = _KArray[0];

        // Remove the head of the binary heap.
        BOOLEAN isRemoved = _KArray.Remove(0);
        KInvariant(isRemoved == TRUE);
        KAssert(_KArray.Count() == 0);

        return TRUE;
    }

    // Set out parameter to head of the binary heap.
    Value = _KArray[0];

    // Move the last item in the array to the head of the binary heap.
    ULONG tailIndex = GetTailIndex();    
    T lastItem = _KArray[tailIndex];
    _KArray[0] = lastItem;

    // Remove the last item in the array.
    _KArray.Remove(tailIndex);

    // Min-heapify the binary heap.
    BubbleDown(0);
    
    return TRUE;
}

// DevNote: Use iterative versus recursive in case we use this in kernel mode.
template <class T> inline
void KPriorityQueue<T>::BubbleUp(__in ULONG Index)
{
    KAssert(Index < _KArray.Count());

    ULONG currentIndex = Index;
    while(currentIndex > 0)
    {
        ULONG parentIndex = GetParentIndex(currentIndex);

        // Using less-than only.
        // if parent <= current, done;
        if (IsLessThan(_KArray[currentIndex], _KArray[parentIndex]) == false)
        {
            break;
        }

        Swap(_KArray[parentIndex], _KArray[currentIndex]);
        currentIndex = parentIndex;
    }
}

// DevNote: Use iterative versus recursive in case we use this in kernel mode.
template <class T> inline
void KPriorityQueue<T>::BubbleDown(__in ULONG Index)
{
    KAssert(Index <= _KArray.Count());

    if (_KArray.Count() == 1)
    {
        return;
    }

    ULONG currentIndex = Index;
    while(currentIndex < _KArray.Count())
    {
        ULONG leftChildIndex = GetLeftChildIndex(currentIndex);
        ULONG rightChildIndex = GetRightChildIndex(currentIndex);

        // If it has no children return.
        if (leftChildIndex >= _KArray.Count())
        {
            return;
        }

        // Find the smaller child.
        ULONG indexOfSmallerChild;
        if (rightChildIndex >= _KArray.Count())
        {
            indexOfSmallerChild = leftChildIndex;
        }
        else
        {
            // Using less-than only.
            indexOfSmallerChild = IsLessThan(_KArray[rightChildIndex], _KArray[leftChildIndex]) ? rightChildIndex : leftChildIndex;
        }

        // Using less-than only.
        // if child >= current, done;
        if (IsLessThan(_KArray[indexOfSmallerChild], _KArray[currentIndex]) == false )
        {
            break;
        }
        
        Swap(_KArray[indexOfSmallerChild], _KArray[currentIndex]);
        currentIndex = indexOfSmallerChild;
    }
}
