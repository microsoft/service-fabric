


/*++
   (c) 2011 by Microsoft Corp.  All Rights Reserved.

   khashtable

   Description:

     Kernel Template Library (KTL)

     Implements fast O(1.3) hash table lookups.  Collisions are handled
     by simple overflow chaining.

   History:
     raymcc        20-Apr-2011        Initial version

--*/

/* This file implements two variations of a hash table container:

   KHashTable
        a. Requires no data structure changes on the part of the user
        b. Can fail to insert if an out-of-memory condition is reached.
        c. Doesn't require the key to be part of the data item, so it can be used as an associative array.

        This type is suitable for POD types, raw pointers, KSharedPtr<> types,
        or types with safe implementations of operator=,operator== for keys and operator= for data items.

   KNodeHashTable
        a. Requires that the hashed element has a LIST_ENTRY available for hash table use.
        b. No-fail insertion into the table
        c. Requires the key to be present in the structure being stored

        Type type is suitable for classes/structs with LIST_ENTRY fields that can be
        acquired by the hash table implementation.

        The key type must implement some type of comparison function.  By default
        this is based on operator==, but alternative comparators can be supplied.

    KSharedHashTable
        An alias for a shareable KHashTable (SPtr)
*/

#pragma once

//  KHashTable
//
//  Implements a hash table with collision resolution based on overflow chaining.
//
//  The item to be hashed must have a KeyType which supports:
//
//      (a) A default constructor which sets the state to a null/empty key
//      (b) operator == or K_EqualityComparisonFunction (for key comparison) 
//      (c) operator =   (for key assignment)
//      (d) A K_HashFunction overload to compute a ULONG hash value from the key. See examples at bottom of file.
//
//  The DataType must support
//      (a) operator =
//
//  DataType elements should either be raw pointers, KSharedPtr, GUID, or integral types, or
//  some other type that is low-cost to copy.
//
//  In general, the ElfHash() implementation below is a suitable default for most complex data types.
//
//  If a compound key is needed, wrap the key in a struct/class and implement operator==.
//
template <class KeyType, class DataType>
class KHashTable : public KObject<KHashTable<KeyType, DataType>>
{
    K_DENY_COPY(KHashTable);

public:
    typedef KDelegate<ULONG(const KeyType& Key)> HashFunctionType;
    typedef KDelegate<BOOLEAN(__in const KeyType& KeyOne, __in const KeyType& KeyTwo)> EqualityComparisonFunctionType;

    // KHashTable constructor
    //
    // No memory is allocated at construction.
    //
    // Parameters:
    //      Size        The size of the hash table.  Statistically, this should be approximately twice
    //                  as large as the number of expected elements for best performance, and should be
    //                  a prime number.
    //
    // Note that because linked list overflow chaining is implemented, the table cannot become full until
    // an out-of-memory condition occurs.  However, once saturation rises about 60% or so, performance
    // begins to fall off.
    //
    FAILABLE
    KHashTable(
        __in ULONG Size,
        __in HashFunctionType Func,
        __in KAllocator& Alloc
        ) 
        : _Allocator(Alloc)
        , _Table(nullptr)
    {
        KObject<KHashTable<KeyType, DataType>>::SetConstructorStatus(Initialize(Size, Func));
    }

    FAILABLE
    KHashTable(
            __in ULONG Size,
            __in HashFunctionType Func,
            __in EqualityComparisonFunctionType EqualityComparisonFunctionType,
            __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
        , _Table(nullptr)
        , _EqualityComparisonFunction(EqualityComparisonFunctionType)
    {
        KObject<KHashTable<KeyType, DataType>>::SetConstructorStatus(Initialize(Size, Func, EqualityComparisonFunctionType));
    }

    // Default constructor
    //
    // This allows the hash table to be embedded in another object and be initialized during later execution once
    // the values for Initialize() are known.
    //
    NOFAIL
    KHashTable(__in KAllocator& Alloc)
        :   _Allocator(Alloc),
            _Table(nullptr)
    {
        ClearAndZero();
    }

    // Move constructor.
    //
    NOFAIL
    KHashTable(__in KHashTable&& Src)
        :   _Allocator(Src._Allocator)
    {
        this->SetStatus(Src.Status());
        _Table = Src._Table;
        _HashFunction = Src._HashFunction;

        _IsEqualizationComparisionFunctionSet = Src._IsEqualizationComparisionFunctionSet;
        if(Src._IsEqualizationComparisionFunctionSet)
        {
            _EqualityComparisonFunction = Src._EqualityComparisonFunction;
        }

        _Size = Src._Size;
        _Count = Src._Count;
        _TableCursor = Src._TableCursor;
        _NodeCursor = Src._NodeCursor;

        Src._Table = nullptr;
        Src.Reset();
        // This method invokes clear() which removes any entries in the table. Hence setting the table of the source to be null above
        Src.ClearAndZero();
    }

    // Move assignment operator.
    //
    NOFAIL
    KHashTable& operator=(__in KHashTable&& Src)
    {
        if (&Src == this)
        {
            return *this;
        }

        Clear();
        _deleteArray(_Table);
        Reset();

        this->SetStatus(Src.Status());
        _Table = Src._Table;
        _HashFunction = Src._HashFunction;

        _IsEqualizationComparisionFunctionSet = Src._IsEqualizationComparisionFunctionSet;
        if (Src._IsEqualizationComparisionFunctionSet)
        {
            _EqualityComparisonFunction = Src._EqualityComparisonFunction;
        }

        _Size = Src._Size;
        _Count = Src._Count;
        _TableCursor = Src._TableCursor;
        _NodeCursor = Src._NodeCursor;

        Src._Table = nullptr;
        Src.Reset();
        // This method invokes clear() which removes any entries in the table. Hence setting the table of the source to be null above
        Src.ClearAndZero();

        return *this;
    }

    // Initialize
    //
    // Used to initialize a hash table that used the default simple NOFAIL constructor.
    // This can be called some time after construction.
    //
    NTSTATUS
    Initialize(__in ULONG Size,
			   __in HashFunctionType Func
			  )
    {
        ClearAndZero();
        _Size = Size;
        _HashFunction = Func;
        _IsEqualizationComparisionFunctionSet = FALSE;
        return InitializeTable();
    }

    NTSTATUS
    Initialize(
        __in ULONG Size,
        __in HashFunctionType Func,
        __in EqualityComparisonFunctionType EqualityComparitionFunc
    )
    {
        ClearAndZero();
        _Size = Size;
        _HashFunction = Func;
        _IsEqualizationComparisionFunctionSet = TRUE;
        _EqualityComparisonFunction = EqualityComparitionFunc;
        return InitializeTable();
    }

    // Destructor
    //
    ~KHashTable()
    {
        Clear();
        _deleteArray(_Table);
    }


    // Get
    //
    // Finds the data item based on the key.
    //
    // Parameters:
    //   Key            The key to use to locate the item.
    //                  The KeyType must implement operator== or EqualityComparisonFunction has to be provided
    //   Data           Receives the located data item.
    //
    // Return values:
    //    STATUS_SUCCESS        The item was located and returned.
    //    STATUS_NOT_FOUND      The item was not present.
    //
    NTSTATUS
    Get(__in  const KeyType& Key,
        __out DataType& Data
        ) const
    {
        DataType* dataPtr;
        NTSTATUS status = GetPtr(Key, dataPtr);
        if (NT_SUCCESS(status))
        {
            Data = *dataPtr;
        }

        return status;
    }

    // GetPtr
    //
    // Get a pointer to data item based on the key.
    //
    // Parameters:
    //   Key            The key to use to locate the item.
    //                  The KeyType must implement operator== or EqualityComparisonFunction must be provided.
    //   Data           Received pointer to the located data item.
    //
    // Return values:
    //    STATUS_SUCCESS        The item was located and returned.
    //    STATUS_NOT_FOUND      The item was not present.
    //
    NTSTATUS
    GetPtr(
        _In_  const KeyType& Key,
        _Out_ DataType* & Data
        ) const
    {
#ifdef KTL_HASH_STATS
        ULONG SearchCount = 0;
#endif
        ULONG Locus = _HashFunction.ConstCall(Key) % _Size;
        _HashEl* Node = _Table[Locus];
        for ( ; Node; Node = Node->_Next)
        {
            if (AreEqual(Node->_Key, Key))
            {
                Data = &Node->_Data;

#ifdef KTL_HASH_STATS
                if (SearchCount > _WorstSearch)
                {
                    _WorstSearch = SearchCount;
                }
                if (SearchCount == 0)
                {
                    _Search1++;
                }
                else if (SearchCount == 1)
                {
                    _Search2++;
                }
                else
                {
                    _Search3++;
                }
#endif

                return STATUS_SUCCESS;
            }
#ifdef KTL_HASH_STATS
            SearchCount++;
#endif
        }
        return STATUS_NOT_FOUND;
    }

    // Put
    //
    // Adds or updates the item.
    //
    // Parameters:
    //      Key           The key under which to store the data item
    //      Data          The data item to store.
    //      ForceUpdate   If TRUE, always write the item, even if it is an update.
    //                    If FALSE, only create, but don't update.
    //      PreviousValue Retrieves the previously existing value if one exists.
    //                    Typically used in combination with ForceUpdate==FALSE.
    //
    // Return values:
    // Return values:
    //      STATUS_SUCCESS - Item was inserted and there was no previous item with
    //          the same key.
    //      STATUS_OBJECT_NAME_COLLISION - Item was not inserted because there was an
    //          existing item with the same key and ForceUpdate was false.
    //      STATUS_OBJECT_NAME_EXISTS - Item replaced a previous item.
    //      STATUS_INSUFFICIENT_RESOURCES - Item was not inserted.
    //
    NTSTATUS
    Put(
        __in  const KeyType&  Key,
        __in  const DataType& Data,
        __in  BOOLEAN   ForceUpdate = TRUE,
        __out_opt DataType* PreviousValue = nullptr
        )
    {
        ULONG HashValue = _HashFunction(Key);
        ULONG Locus = HashValue % _Size;
        _HashEl* Node = _Table[Locus];
        _HashEl* Prev = nullptr;
        for ( ; Node; Prev = Node, Node = Node->_Next)
        {
            if (AreEqual(Node->_Key, Key))
            {
                if (PreviousValue)
                {
                    *PreviousValue = Node->_Data;
                }
                if (ForceUpdate)
                {
                    Node->_Data = Data;
                    return STATUS_OBJECT_NAME_EXISTS;
                }
                else
                {
                    return STATUS_OBJECT_NAME_COLLISION;
                }
            }
        }

        // If here, we didn't find a pre-existing node
        // with the specified key.  Add a new one.
        //

        _HashEl* NewNode = _new (KTL_TAG_KHASHTABLE, _Allocator) _HashEl(Key, Data, HashValue);
        if (!NewNode)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Prev)
        {
            Prev->_Next = NewNode;
        }
        else
        {
            _Table[Locus] = NewNode;
        }

        _Count++;

        return STATUS_SUCCESS;
    }

    // Remove
    //
    // Removes the specified Key and its associated data item from the table
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_NOT_FOUND
    //
    NTSTATUS
    Remove(__in const KeyType& Key,
           __out  DataType* Item = nullptr
          )
    {
        ULONG Locus = _HashFunction(Key) % _Size;
        _HashEl* Node = _Table[Locus];
        _HashEl* Prev = nullptr;
        for ( ; Node; Prev = Node, Node = Node->_Next)
        {
            if (AreEqual(Node->_Key, Key))
            {
                if (Prev)
                {
                    Prev->_Next = Node->_Next;
                }
                else
                {
                    _Table[Locus] = Node->_Next;
                }
                if (Item)
                {
                    *Item = Node->_Data;
                }
                _delete(Node);
                _Count--;
                return STATUS_SUCCESS;
            }
        }
        return STATUS_NOT_FOUND;
    }


    // Clear
    //
    // Remove all entries from the table.
    //
    VOID
    Clear()
    {
        if (_Table)
        {
            for (ULONG Locus = 0; Locus < _Size; Locus++)
            {
                _HashEl* Node = _Table[Locus];
                while (Node)
                {
                   _HashEl* Tmp = Node->_Next;
                    _delete(Node);
                    _Count--;
                    Node = Tmp;
                }
                _Table[Locus] = nullptr;
            }
        }

        _Count = 0;
#ifdef KTL_HASH_STATS
        _WorstSearch = 0;
        _Search1 = _Search2 = _Search3 = 0;
#endif
    }

    // ContainsKey
    //
    // Checks the data item for existence based on the key.
    //
    // Parameters:
    //   Key            The key to use to locate the item.
    //                  The KeyType must implement operator== or EqualityComparisonFunction must be provided.
    //
    // Return values:
    //    TRUE          The item was located and returned.
    //    FALSE        The item was not present.
    //
    BOOLEAN
    ContainsKey(__in  KeyType& Key) const
    {
#ifdef KTL_HASH_STATS
        ULONG SearchCount = 0;
#endif
        ULONG Locus = _HashFunction.ConstCall(Key) % _Size;
        _HashEl* Node = _Table[Locus];
        for ( ; Node; Node = Node->_Next)
        {
            if (AreEqual(Node->_Key, Key))
            {

#ifdef KTL_HASH_STATS
                if (SearchCount >= _WorstSearch)
                {
                    _WorstSearch = (SearchCount + 1);
                }
                if (SearchCount == 0)
                {
                    _Search1++;
                }
                else if (SearchCount == 1)
                {
                    _Search2++;
                }
                else
                {
                    _Search3++;
                }
#endif

                return TRUE;
            }
#ifdef KTL_HASH_STATS
            SearchCount++;
#endif
        }
        return FALSE;
    }

    // Count
    //
    // Returns the total population of the table.
    //
    __forceinline ULONG
    Count() const
    {
        return _Count;
    }

    // Size
    //
    // Returns the current total hash table size (not population or overflow chain records)
    __forceinline ULONG
    Size() const
    {
        return _Size;
    }

    // Saturation
    //
    // Returns the saturation of the table in percent.  This should not rise above 60%,
    // or the table is sized too small.
    //
    __forceinline ULONG
    Saturation() const
    {
        ULONGLONG ReturnValue = ULONGLONG(_Count) * ULONGLONG(100);
        ReturnValue /= (ULONGLONG(_Size));
        return ULONG(ReturnValue);
    }

    __forceinline ULONG
    Saturation(ULONG WouldBeSize) const
    {
        ULONGLONG ReturnValue = ULONGLONG(_Count) * ULONGLONG(100);
        ReturnValue /= (ULONGLONG(WouldBeSize));
        return ULONG(ReturnValue);
    }

    // GetStatistics
    //
    // Returns search statistics since construction or the last call to Clear().
    //
    // Parameters:
    //      Search1     The number of searches satisfied on the first hit
    //      Search2     The number of searches satisfied on the second hit (first overflow)
    //      Search3     The number of searches satisfied on the 3rd or more hit
    //      Worst       The single worst search in the table
    //
#ifdef KTL_HASH_STATS
    void
    GetStatistics(
        __out ULONG& Search1,
        __out ULONG& Search2,
        __out ULONG& Search3,
        __out ULONG& Worst
        ) const
    {
        Search1 = _Search1;
        Search2 = _Search2;
        Search3 = _Search3;
        Worst = _WorstSearch;
    }
#endif

    // Resize
    //
    // Resizes the hash table.
    // Parameters:
    //      NewSize         This typically should be 50% larger than the previous size,
    //                      but still a prime number.
    //
    //
    NTSTATUS
    Resize(__in ULONG NewSize)
    {
        // Create the new hash bucket table
        _HashEl** tmpTable = _newArray<_HashEl*>(KTL_TAG_KHASHTABLE, _Allocator, NewSize);
        if (tmpTable == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(tmpTable, sizeof(_HashEl*) * NewSize);

        // Redistribute all elements to the new hash bucket table
        for (ULONG ix = 0; ix < _Size; ix++)
        {
            _HashEl*    nextElement = _Table[ix];
            while (nextElement != nullptr)
            {
                _HashEl&    currentElement = *nextElement;
                nextElement = currentElement._Next;

                ULONG   newLocus = currentElement._HashValue % NewSize;
                currentElement._Next = tmpTable[newLocus];
                tmpTable[newLocus] = &currentElement;
            }
        }

        #ifdef KTL_HASH_STATS
           _Search1 = _Search2 = _Search3 = _WorstSearch = 0;
        #endif

        Reset();
        _deleteArray(_Table);
        _Table = tmpTable;
        _Size = NewSize;
        return STATUS_SUCCESS;
    }

    // Reset
    //
    // Begins an iteration sequence over the hash table. Note that the elements are unordered
    // by key value due to the nature of hash tables.
    //
    VOID
    Reset()
    {
        _NodeCursor = nullptr;
        _TableCursor = -1;
    }

    // Next
    //
    // Retrieves the next key/data pair in the table during an enumeration started by Reset().
    //
    NTSTATUS
    Next(__out KeyType& Key,
         __out DataType& Data
        )
    {
        const KeyType* keyPtr;
        DataType* dataPtr;
        NTSTATUS status = NextPtr(keyPtr, dataPtr);
        if (NT_SUCCESS(status))
        {
            Key = *keyPtr;
            Data = *dataPtr;
        }

        return status;
    }

    // NextPtr
    //
    // Retrieves pointer to next key/data pair in the table during an enumeration started by Reset().
    // Deprecated - Unsafe as it returns a non-const pointer.
    //
    NTSTATUS
    NextPtr(_Out_ KeyType* & Key,
            _Out_ DataType* & Data
        )
    {
        return NextPtr(const_cast<const KeyType* &>(Key), Data);
    }

    // NextPtr
    //
    // Retrieves pointer to next key/data pair in the table during an enumeration started by Reset().
    //
    NTSTATUS
    NextPtr(_Out_ const KeyType* & Key,
            _Out_ DataType* & Data
           )
    {
        if (!_NodeCursor)
        {
            while ((ULONG)(++_TableCursor) < _Size)
            {
                _HashEl* El = _Table[_TableCursor];
                if (!El)
                {
                    continue;
                }
                _NodeCursor = El;
                break;
            }
            if (!_NodeCursor)
            {
                return STATUS_NO_MORE_ENTRIES;
            }
        }

        Key = &_NodeCursor->_Key;
        Data = &_NodeCursor -> _Data;
        _NodeCursor = _NodeCursor->_Next;

        return STATUS_SUCCESS;
    }

private:
    __forceinline BOOLEAN AreEqual(
        __in KeyType const& keyOne, 
        __in KeyType const& keyTwo) const
    {
        if (_IsEqualizationComparisionFunctionSet == FALSE)
        {
            return keyOne == keyTwo;
        }

        return _EqualityComparisonFunction.ConstCall(keyOne, keyTwo);
    }

    VOID
    ClearAndZero()
    {
        Clear();
        _Table = nullptr;
        _Size = 0;
        _Count = 0;
#ifdef KTL_HASH_STATS
        _WorstSearch = 0;
        _Search1 = _Search2 = _Search3 = 0;
#endif
    }

    NTSTATUS InitializeTable()
    {
        _Table = _newArray<_HashEl*>(KTL_TAG_KHASHTABLE, _Allocator, _Size);
        if (!_Table)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(_Table, sizeof(_HashEl*)*_Size);
        return STATUS_SUCCESS;
    }

    class _HashEl
    {
    public:
        const KeyType   _Key;
        DataType        _Data;
        _HashEl*        _Next = nullptr;
        const ULONG     _HashValue;

       _HashEl(const KeyType&  KeyValue, const DataType& DataValue, ULONG HashValue) : 
            _Key (KeyValue),
            _Data(DataValue),
            _HashValue(HashValue)
       {
       }
    };

    _HashEl**    _Table;
    HashFunctionType _HashFunction;
    EqualityComparisonFunctionType _EqualityComparisonFunction;
    BOOLEAN _IsEqualizationComparisionFunctionSet = false;

    ULONG        _Size;
    ULONG        _Count;

    LONG         _TableCursor;
    KAllocator&  _Allocator;
    _HashEl*     _NodeCursor;

    // Statistics
#ifdef KTL_HASH_STATS
    mutable ULONG        _Search1;
	mutable ULONG        _Search2;
	mutable ULONG        _Search3;
	mutable ULONG        _WorstSearch;
#endif
};

template<typename K, typename V>
using KSharedHashTable = KSharedType<KHashTable<K,V>>;

//  KNodeHashTable
//
//  Implements a hash table with collision resolution based on overflow chaining.
//
template <class KeyType, class ItemType>
class KNodeHashTable : public KObject<KNodeHashTable<KeyType, ItemType>>
{
public:
    typedef KDelegate<ULONG(const KeyType& Key)> HashFunctionType;

    // KNodeHashTable constructor
    FAILABLE
    KNodeHashTable(
        __in ULONG Size,
        __in HashFunctionType Func,
        __in ULONG KeyOffset,
        __in ULONG LinkOffset,
        __in KAllocator& Alloc
        )
        :   _Table(nullptr)
    {
        this->SetConstructorStatus(Initialize(Size, Func, KeyOffset, LinkOffset, Alloc));
    }

    // Default constructor
    //
    // This allows the hash table to be embedded in another object and be initialized during later execution once
    // the values for Initialize() are known.
    //
    NOFAIL
    KNodeHashTable()
        :   _Table(nullptr)
    {
        ClearAndZero();
    }

    // Initialize
    //
    // Used to initialize a hash table that used the default simple NOFAIL constructor.
    // This can be called some time after construction.
    //
    NTSTATUS
    Initialize(
        __in ULONG Size,
        __in HashFunctionType Func,
        __in ULONG KeyOffset,
        __in ULONG LinkOffset,
        __in KAllocator& Alloc
        )
    {
        ClearAndZero();
        _Size = Size;
        _HashFunction = Func;
        _KeyOffset = KeyOffset;
        _LinkOffset = LinkOffset;
        return InitializeTable(Alloc);
    }

    // Destructor
    //
    ~KNodeHashTable()
    {
        Clear();
        _deleteArray(_Table);
    }

    //
    // Helper to get the object pointer from the link pointer
    //
    inline ItemType* GetItemPtr(KListEntry* Link) const
    {
        return reinterpret_cast<ItemType*>(PUCHAR(Link) - _LinkOffset);
    }

    //
    // Helper to get the key pointer from the object pointer.
    //
    inline KeyType* GetKeyPtr(ItemType* Item) const
    {
        return reinterpret_cast<KeyType*>(PUCHAR(Item) + _KeyOffset);
    }

    //
    //  Helper to get the link pointer from the object pointer.
    //
    inline KListEntry* GetLinkPtr(ItemType* Item) const
    {
        return reinterpret_cast<KListEntry*>(PUCHAR(Item) + _LinkOffset);
    }

    //
    //  Helper to get/set hash in an ItemType link entry.
    //
    inline ULONG GetHashValue(KListEntry* ForLink) const
    {
        return (ULONG)((ULONGLONG)(ForLink->Blink));
    }

    inline void SetHashValue(KListEntry* ForLink, ULONG HashValue)
    {
        ForLink->Blink = (KListEntry*)((ULONGLONG)HashValue);
    }

    // Get
    //
    // Finds the item based on the key.
    //
    // Parameters:
    //   Key            The key to use to locate the item.
    //                  The KeyType must implement operator==.
    //   Item           Receives the located tem.
    //
    // Return values:
    //    STATUS_SUCCESS        The item was located and returned.
    //    STATUS_NOT_FOUND      The item was not present.
    //
    NTSTATUS
    Get(__in   KeyType& Key,
        __out  ItemType*& Item
        ) const
    {
#ifdef KTL_HASH_STATS
        ULONG SearchCount = 0;
#endif
        ULONG Locus = _HashFunction.ConstCall(Key) % _Size;
        for (KListEntry* Link = &_Table[Locus]; Link; Link = (KListEntry*)Link->Flink)
        {
            if (Link->Flink)
            {
                KeyType* NodeKey = GetKeyPtr(GetItemPtr((KListEntry*)Link->Flink));
                if (*NodeKey == Key)
                {
                    Item = GetItemPtr((KListEntry*) Link->Flink);

#ifdef KTL_HASH_STATS
                    if (SearchCount >= _WorstSearch)
                    {
                        _WorstSearch = (SearchCount + 1);
                    }
                    if (SearchCount == 0)
                    {
                        _Search1++;
                    }
                    else if (SearchCount == 1)
                    {
                        _Search2++;
                    }
                    else
                    {
                        _Search3++;
                    }
#endif
                    return STATUS_SUCCESS;

                }   // If key match
            }       //

#ifdef KTL_HASH_STATS
            SearchCount++;
#endif
        }
        return STATUS_NOT_FOUND;
    }

    // Put
    //
    // Adds or updates the item.
    //
    // Parameters:
    //      Item          The data item to store.
    //      ForceUpdate   If TRUE, always write the item, even if it is an update.
    //                    If FALSE, only create, but don't update.
    //      PreviousValue Retrieves the previously existing value if one exists.
    //                    Typically used in combination with ForceUpdate==FALSE.
    //
    // Return values:
    //      STATUS_SUCCESS - Item was inserted and there was no previous item with
    //          the same key.
    //      STATUS_OBJECT_NAME_COLLISION - Item was not inserted because there was an
    //          existing item with the same key and ForceUpdate was false.
    //      STATUS_OBJECT_NAME_EXISTS - Item replaced a previous item.
    //
    NTSTATUS
    Put(__in  ItemType*  Item,
        __in  BOOLEAN    ForceUpdate = TRUE,
        __out_opt ItemType** PreviousValue = nullptr
        )
    {
        KeyType* PutKey = GetKeyPtr(Item);
        ULONG HashValue = _HashFunction(*PutKey);
        ULONG Locus =  HashValue % _Size;
        KListEntry* Link = &_Table[Locus];

        for ( ; Link->Flink; Link = (KListEntry *)Link->Flink)
        {
            ItemType* ItemPtr = GetItemPtr((KListEntry*)Link->Flink);
            KeyType*  KeyPtr  = GetKeyPtr(GetItemPtr((KListEntry*)Link->Flink));

            if (*KeyPtr == *PutKey)
            {
                if (PreviousValue)
                {
                    *PreviousValue = ItemPtr;
                }

                if (ForceUpdate)
                {
                    KListEntry* NewLink = GetLinkPtr(Item);
                    KListEntry* OldLink = (KListEntry*) Link->Flink;
                    NewLink->Flink = (KListEntry*)OldLink->Flink;
                    SetHashValue(NewLink, HashValue);
                    Link->Flink = NewLink;

                    OldLink->Flink = nullptr; // Make sure the old links are nullified
                    OldLink->Blink = nullptr; // "

                    return STATUS_OBJECT_NAME_EXISTS;
                }
                else
                {
                    return STATUS_OBJECT_NAME_COLLISION;
                }
            }
        }

        if (PreviousValue)
        {
            *PreviousValue = nullptr;
        }

        // If here, we didn't find a pre-existing node
        // with the specified key, so we add a new one. Link
        // is pointing at the right place based on the termination
        // condition of the previous loop.
        //
        Link->Flink = GetLinkPtr(Item);
        Link->Flink->Flink = nullptr;
        SetHashValue((KListEntry*)(Link->Flink), HashValue);

        _Count++;

        return STATUS_SUCCESS;
    }

    // Remove
    //
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_NOT_FOUND
    //
    NTSTATUS
    Remove(__in ItemType* Item)
    {
        KeyType* PutKey = GetKeyPtr(Item);
        ULONG Locus = _HashFunction(*PutKey) % _Size;
        KListEntry* Link = &_Table[Locus];
        KListEntry* OldLink = GetLinkPtr(Item);

        for ( ; Link->Flink; Link = (KListEntry *)Link->Flink)
        {
            if (Link->Flink == OldLink)
            {

                KListEntry* OldSuccessor = (KListEntry*) OldLink->Flink;
                OldLink->Flink = nullptr; // Make sure the old links are nullified
                OldLink->Blink = nullptr; // "
                Link->Flink = OldSuccessor;
                _Count--;
                return STATUS_SUCCESS;
            }
        }

        return STATUS_NOT_FOUND;
    }


    // Clear
    //
    // Remove all entries from the table.
    //
    VOID
    Clear()
    {
        PLIST_ENTRY Entry;

        if (_Table)
        {
            for (ULONG i = 0; i < _Size; i++)
            {
                //
                // Null out the list entries for the items on the list.
                //

                while (_Table[i].Flink != nullptr )
                {
                    Entry = _Table[i].Flink;
                    _Table[i].Flink = Entry->Flink;
                    Entry->Blink = Entry->Flink = nullptr;
                }

                _Table[i].Flink = nullptr;
                _Table[i].Blink = nullptr;
            }
        }

        _Count = 0;
#ifdef KTL_HASH_STATS
        _WorstSearch = 0;
        _Search1 = _Search2 = _Search3 = 0;
#endif
    }

    // ContainsKey
    //
    // Returns a value that indicates whether the specified key is in the hashtable.
    //
    BOOLEAN ContainsKey(__in KeyType & Key) const
    {
        ItemType* Item = nullptr;

        NTSTATUS Status = Get(Key, Item);

        return NT_SUCCESS(Status);
    }

    // Count
    //
    // Returns the total population of the table.
    //
    ULONG
    Count() const
    {
        return _Count;
    }

    // IsEmpty
    //
    // Returns true if the hashtable is empty (Count() == 0), false otherwise.
    //
    BOOLEAN
    IsEmpty() const
    {
        return _Count == 0;
    }

    // Size
    //
    // Returns the current total hash table size (not population or overflow chain records)
    ULONG
    Size() const
    {
        return _Size;
    }

    // Saturation
    //
    // Returns the saturation of the table in percent.  This should not rise above 60%,
    // or the table is sized too small.
    //
    ULONG
    Saturation() const
    {
		ULONGLONG ReturnValue = ULONGLONG(_Count) * ULONGLONG(100);
		ReturnValue /= (ULONGLONG(_Size));
		return ULONG(ReturnValue);
	}

	ULONG
	Saturation(ULONG WouldBeSize) const
	{
		ULONGLONG ReturnValue = ULONGLONG(_Count) * ULONGLONG(100);
		ReturnValue /= (ULONGLONG(WouldBeSize));
		return ULONG(ReturnValue);
	}

    // GetStatistics
    //
    // Returns search statistics since construction or the last call to Clear().
    //
    // Parameters:
    //      Search1     The number of searches satisfied on the first hit
    //      Search2     The number of searches satisfied on the second hit (first overflow)
    //      Search3     The number of searches satisfied on the 3rd or more hit
    //      Worst       The single worst search in the table
    //
#ifdef KTL_HASH_STATS
    void
    GetStatistics(
        __out ULONG& Search1,
        __out ULONG& Search2,
        __out ULONG& Search3,
        __out ULONG& Worst
        ) const
    {
        Search1 = _Search1;
        Search2 = _Search2;
        Search3 = _Search3;
        Worst = _WorstSearch;
    }
#endif

    // Resize
    //
    // Resizes the hash table.
    // Parameters:
    //      NewSize         This typically should be 50% larger than the previous size,
    //                      but still a prime number.
    //
    //
    NTSTATUS
    Resize(__in ULONG NewSize)
    {
        // Create the new hash bucket table
        KListEntry* tmpTable = _newArray<KListEntry>(KTL_TAG_KHASHTABLE, *_Allocator, NewSize);
        if (tmpTable == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(tmpTable, sizeof(KListEntry) * NewSize);

        // Redistribute all elements to the new hash bucket table
        for (ULONG ix = 0; ix < _Size; ix++)
        {
            KListEntry*    nextElement = (KListEntry*)(_Table[ix].Flink);
            while (nextElement != nullptr)
            {
                KListEntry&     currentElement = *nextElement;
                nextElement = (KListEntry*)(currentElement.Flink);

                ULONG   newLocus = GetHashValue(&currentElement) % NewSize;
                currentElement.Flink = tmpTable[newLocus].Flink;
                tmpTable[newLocus].Flink = &currentElement;
            }

            _Table[ix].Flink = _Table[ix].Blink = nullptr;
        }

        #ifdef KTL_HASH_STATS
           _Search1 = _Search2 = _Search3 = _WorstSearch = 0;
        #endif

        Reset();
        _deleteArray(_Table);
        _Table = tmpTable;
        _Size = NewSize;
        return STATUS_SUCCESS;
    }

    // Reset
    //
    // Begins an iteration sequence over the hash table. Note that the elements are unordered
    // by key value due to the nature of hash tables.
    //
    VOID
    Reset()
    {
        _NodeCursor = nullptr;
        _TableCursor = -1;
    }

    // Next
    //
    // Retrieves the next key/data pair in the table during an enumeration started by Reset().
    //
    NTSTATUS
    Next(__out ItemType*& Item)
    {
        if (!_NodeCursor)
        {
            while ((ULONG)(++_TableCursor) < _Size)
            {
                _NodeCursor = (KListEntry*) _Table[_TableCursor].Flink;
                if (_NodeCursor)
                {
                    break;
                }
            }
            if (!_NodeCursor)
            {
                return STATUS_NO_MORE_ENTRIES;
            }
        }

        Item = GetItemPtr((KListEntry*) _NodeCursor);
        _NodeCursor = (KListEntry*) _NodeCursor->Flink;

        return STATUS_SUCCESS;
    }

private:
    VOID
    ClearAndZero()
    {
        Clear();
        _Table = nullptr;
        _Allocator = nullptr;
        _Size = 0;
        _Count = 0;
        _LinkOffset = 0;
        _KeyOffset = 0;
#ifdef KTL_HASH_STATS
        _WorstSearch = 0;
        _Search1 = _Search2 = _Search3 = 0;
#endif
    }

    NTSTATUS InitializeTable(KAllocator& Alloc)
    {
        _Allocator = &Alloc;
        _Table = _newArray<KListEntry>(KTL_TAG_KHASHTABLE, Alloc, _Size);
        if (!_Table)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        return STATUS_SUCCESS;
    }


    KAllocator*      _Allocator;
    KListEntry*      _Table;
    HashFunctionType _HashFunction;

    ULONG        _Size;
    ULONG        _Count;
    ULONG        _KeyOffset;
    ULONG        _LinkOffset;

    LONG         _TableCursor;
    KListEntry*  _NodeCursor;

    // Statistics
#ifdef KTL_HASH_STATS
    mutable ULONG        _Search1;
	mutable ULONG        _Search2;
	mutable ULONG        _Search3;
	mutable ULONG        _WorstSearch;
#endif
};




// Default hash functions for various item types.
//
inline ULONG
K_DefaultHashFunction(const ULONG& Key)
{
    return Key;
}

inline ULONG
K_DefaultHashFunction(const ULONGLONG& Key)
{
    return ULONG(Key ^ (Key >> 32));
}

inline ULONG
K_DefaultHashFunction(__in const LONGLONG& Key)
{
    return K_DefaultHashFunction((const ULONGLONG&)Key);
}

inline ULONG
K_DefaultHashFunction(const GUID& Key)
{
    ULONG* Ptr = PULONG(&Key);
    ULONG Hash = *Ptr++;
    Hash ^= *Ptr++;
    Hash ^= *Ptr++;
    Hash ^= *Ptr;
    return Hash;
}

// Custom hash function can be mapped to this.
// For long keys, usually the whole key is not
// required to make a good hash.
//
inline ULONG
K_DefaultHashFunction(__in const KMemRef& Mem)
{
    return KChecksum::Crc32(Mem._Address, Mem._Size, 0);
}

inline ULONG
K_DefaultHashFunction(__in const KWString& Str)
{
    KMemRef Mem;
    Mem._Address = PWSTR((KWString&)Str);
    Mem._Size = Str.Length();
    return K_DefaultHashFunction(Mem);
}

#ifndef PLATFORM_UNIX
inline ULONG
K_DefaultHashFunction(__in KStringViewA& Str)
{
    KMemRef Mem;
    Mem._Address = PSTR(Str);
    Mem._Size = Str.Length();
    return K_DefaultHashFunction(Mem);
}
#endif

inline ULONG
K_DefaultHashFunction(__in KStringView& StrView)
{
    KMemRef Mem;
    Mem._Address = PWSTR(StrView);
    Mem._Size = StrView.Length() * 2;
    return K_DefaultHashFunction(Mem);
}

inline ULONG
K_DefaultHashFunction(__in const KStringView& StrView)
{
    KMemRef Mem;
    Mem._Address = PWSTR(StrView);
    Mem._Size = StrView.Length() * 2;
    return K_DefaultHashFunction(Mem);
}

inline ULONG
K_DefaultHashFunction(__in KString::SPtr const & StrSPtr)
{
    return K_DefaultHashFunction(*StrSPtr);
}

inline ULONG
K_DefaultHashFunction(__in KString::CSPtr const & StrCSPtr)
{
    return K_DefaultHashFunction(*StrCSPtr);
}

inline ULONG
K_DefaultHashFunction(__in KUriView& UriView)
{
    KStringView strView = UriView;
    return K_DefaultHashFunction(strView);
}

inline ULONG
K_DefaultHashFunction(__in const KUriView& UriView)
{
    KStringView strView = UriView.Get(KUriView::eRaw);
    return K_DefaultHashFunction(strView);
}

inline ULONG
K_DefaultHashFunction(__in KUri::SPtr const & UriView)
{
    KStringView strView = *UriView;
    return K_DefaultHashFunction(strView);
}

inline ULONG
K_DefaultHashFunction(__in KUri::CSPtr const & UriView)
{
    return K_DefaultHashFunction(*UriView);
}
