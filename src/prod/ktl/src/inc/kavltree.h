/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kavltree

    Description:
      Kernel Tempate Library (KTL): KAvlTree

      A templatized wrapper of the AVL tree in the DDK.

    History:
      raymcc          16-Aug-2010         Initial version.
      pengli          3-Sept-2010         Initial implementation.

--*/

#define KAVL_INVALID_ITERATOR_INDEX     ((ULONG)(-1))

template <class T, typename KeyType, ULONG MaxIterators = 8>
class KAvlTree : public KObject<KAvlTree<T, KeyType, MaxIterators>>
{
    K_DENY_COPY(KAvlTree);

public:
    typedef KAvlTree<T, KeyType, MaxIterators> KAvlTreeType;
    
    class Iterator
    {        
    public:
        ~Iterator()
        {
            if (_AvlTree && _IteratorIndex < MaxIterators)
            {
                ASSERT(!_AvlTree->_Iterators[_IteratorIndex].Free);
                
                _AvlTree->_Iterators[_IteratorIndex].Zero();
                _AvlTree->_Iterators[_IteratorIndex].Free = TRUE;
            }

            _AvlTree = NULL;
            _IteratorIndex = KAVL_INVALID_ITERATOR_INDEX;
        }

        //
        // Test if the iterator points to a valid node in the tree. 
        // If it points to a node that has been deleted or does not exist,
        // this iterator is invalid.
        //
        BOOLEAN IsValid() const
        {
            if (!_AvlTree || 
                _IteratorIndex >= MaxIterators || 
                !_AvlTree->_Iterators[_IteratorIndex].Node)
            {
                return FALSE;
            }

            return TRUE;
        }        

        //
        // Move the iterator to the first element in the tree.
        //        
        BOOLEAN  First()
        {
            if (!_AvlTree || _IteratorIndex >= MaxIterators)
            {
                return FALSE;
            }

            AvlIterator* iter = &_AvlTree->_Iterators[_IteratorIndex];
            iter->Zero();

            PVOID p = RtlEnumerateGenericTableWithoutSplayingAvl(&_AvlTree->_AvlTable, &iter->RestartKey);
            if (!p)
            {
                return FALSE;
            }

            iter->Node = reinterpret_cast<AvlNode*>(p);
            iter->DeleteCount = _AvlTree->_AvlTable.DeleteCount;
            
            return TRUE;
        }

        //
        // Move the iterator to the next element in the tree.
        //        
        BOOLEAN Next()
        {
            if (!IsValid())
            {
                return FALSE;
            }

            AvlIterator*    iter = &_AvlTree->_Iterators[_IteratorIndex];            
            ULONG           nextFlag = TRUE;
            
            PVOID p = RtlEnumerateGenericTableLikeADirectory(&_AvlTree->_AvlTable, NULL, NULL, 
                        nextFlag, &iter->RestartKey, &iter->DeleteCount, iter->Node);
            iter->Node = reinterpret_cast<AvlNode*>(p);            
            if (iter->Node)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        T& Get() const
        {
            ASSERT(IsValid());
            
            AvlNode* node = _AvlTree->_Iterators[_IteratorIndex].Node;
            return node->Object;
        }

        const KeyType& GetKey() const
        {
            ASSERT(IsValid());
            
            AvlNode* node = _AvlTree->_Iterators[_IteratorIndex].Node;
            return *(static_cast<KeyType*>(node));
        }        

        Iterator& operator=(
           __in Iterator& Src
           )
        {
            if (this != &Src)
            {   
                // This iterator and Src must occupy valid slots.
                ASSERT(_AvlTree && _IteratorIndex < MaxIterators);
                ASSERT(Src._IteratorIndex < MaxIterators);          
                
                //
                // Assignment is only allowed between iterators of the same AVL tree.
                //
                ASSERT(_AvlTree == Src._AvlTree);

                //
                // Points this iterator to the node pointed to by Src.
                //                
                if (_AvlTree && (_IteratorIndex < MaxIterators) && (Src._IteratorIndex < MaxIterators))
                {
                    _AvlTree->_Iterators[_IteratorIndex] = _AvlTree->_Iterators[Src._IteratorIndex];
                }
            }

            return *this;            
        }

        // Move constructor
        NOFAIL Iterator(
           __in Iterator&& Src
           ) : _AvlTree(Src._AvlTree), _IteratorIndex(Src._IteratorIndex)
        {
            Src._AvlTree = NULL;
            Src._IteratorIndex = KAVL_INVALID_ITERATOR_INDEX;
        }        
              
    private:  
        friend KAvlTreeType;

        //
        // Hide ctors. A user can only construct a KAvlTreeType::Iterator object
        // by either Find() or GetIterator() so that we can constrol
        // the number of outstanding iterators.
        //

        NOFAIL Iterator() : _AvlTree(NULL), _IteratorIndex(KAVL_INVALID_ITERATOR_INDEX)
        {
        } 

        NOFAIL Iterator(
           __in const Iterator& Src
           );
        
        NOFAIL Iterator(__in KAvlTreeType* AvlTree, __in ULONG IteratorIndex, __in BOOLEAN MoveToFirst) :
            _AvlTree(AvlTree), _IteratorIndex(IteratorIndex)
        {
            //
            // Assert this iterator is valid. 
            //
            ASSERT(_AvlTree && _IteratorIndex < MaxIterators);

            // Move the iterator to the first node in the tree if possible.
            if (MoveToFirst)
            {
                First();
            }
        }
        
        KAvlTreeType*   _AvlTree;
        ULONG           _IteratorIndex;
    };

    //
    // Key comparison function. It returns:
    //  < 0: if First is less than Second
    //  0: if First equals to Second
    //  > 0: if First is greater than Second
    //    
    typedef KDelegate<LONG(__in const KeyType& First, __in const KeyType& Second)> KeyComparisonFunc;

    NOFAIL KAvlTree(
		__in KeyComparisonFunc ComparisonFunction, 
        __in KAllocator& Allocator, 
		__in ULONG AllocationTag = KTL_TAG_AVL_TREE) 
		:
        _ComparisonFunction(ComparisonFunction),
        _AllocationTag(AllocationTag),
        _Allocator(Allocator)
    {
        RtlInitializeGenericTableAvl(&_AvlTable, KAvlTreeType::AvlCompareRoutine,
            KAvlTreeType::AvlAllocRoutine, KAvlTreeType::AvlFreeRoutine, this);
    }

    ~KAvlTree()
    {        
        #if DBG

        //
        // Assert that there is no outstanding iterator.
        //

        for (ULONG i = 0; i < MaxIterators; i++)
        {
            ASSERT(_Iterators[i].Free);
        }

        #endif

        RemoveAll();
    }

    //
    // IsEmpty
    //
    // Return values:
    //     TRUE         If the tree is empty.
    //     FALSE        If the tree is not empty.
    //
    BOOLEAN IsEmpty()
    {
        return RtlIsGenericTableEmptyAvl(&_AvlTable);
    }

    //
    // Count
    //
    // Return value:
    //     Returns the number of elements currently in the tree
    //
    ULONG Count()
    {
        return RtlNumberGenericTableElementsAvl(&_AvlTable);
    }

    //
    // Insert
    //
    // Inserts a new element into the tree. If an element with the same key already exists,
    // optionally updates the existing element.
    //
    // Parameters:
    //     Src         The object to be copied.
    //     KeyVal      The value of the key to be associated with this node.
    //                 This object must support copy construction/assignment
    //     ForceUpdate What happens if this key is alread in the tree. If this is set to
    //                 TRUE (the default), the node will be updated with Src.
    //                 If FALSE, this will not update the node
    //  Return value:
    //     STATUS_SUCCESS
    //     STATUS_INSUFFICIENT_RESOURCES
    //     STATUS_OBJECT_NAME_COLLISION         If the key exists and ForceUpdate is FALSE
    //
    NTSTATUS Insert(
        __in T& Src,
        __in const KeyType& KeyVal,
        __in BOOLEAN ForceUpdate = TRUE
        )
    {    
        AvlNode     keyNode(KeyVal);
        BOOLEAN     newElement;
        
        PVOID p = RtlInsertElementGenericTableAvl(&_AvlTable, &keyNode, sizeof(keyNode), &newElement);
        if (!p)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (newElement)
        {
            //
            // The RTL AVL table has allocated a buffer and memcopies the user data
            // into the buffer. 
            //       
            // Use the placement new operator to contruct an AvlNode object 
            // on the buffer.
            //
            
            new(p) AvlNode(KeyVal, Src);
            return STATUS_SUCCESS;
        }

        if (!ForceUpdate)
        {
            return STATUS_OBJECT_NAME_COLLISION;
        }

        AvlNode* existingNode = reinterpret_cast<AvlNode*>(p);
        existingNode->Object = Src;
        return STATUS_SUCCESS;
    }

    //
    // Update 
    //
    // Updates an existing node in the tree. If the tree does not have an existing node 
    // with the specified key, this routine fails with STATUS_NOT_FOUND.
    // optionally updates the existing element.
    //
    // Parameters:
    //     Src         The object to be copied.
    //     KeyVal      The value of the key to be associated with this node.
    //                 This object must support copy construction/assignment
    //  Return value:
    //     STATUS_SUCCESS
    //     STATUS_NOT_FOUND        If the key does not exist
    //
    NTSTATUS Update(
        __in T& Src,
        __in KeyType& KeyVal
        )
    {    
        AvlNode     keyNode(KeyVal);
        
        PVOID p = RtlLookupElementGenericTableAvl(&_AvlTable, &keyNode);
        if (!p)
        {
            return STATUS_NOT_FOUND;
        }

        AvlNode* existingNode = reinterpret_cast<AvlNode*>(p);
        existingNode->Object = Src;
        return STATUS_SUCCESS;
    }
    
    //
    // FindOrInsert
    //
    // Finds the element specified by the key, if it exists, and retrieves it into Existing. If there is no existing
    // element, then the Candidate is inserted.
    //
    // Parameters:
    //    Candidate     The item that may be inserted.
    //    KeyVal        The key to use for the lookup.
    //    Existing      May receive the existing element at that key, if it exists.
    //
    // Candidate and Existing may be set to the same object by the caller.
    //
    // Return:
    //    STATUS_OBJECT_NAME_EXISTS         The object already existed and Existing was populated, Candidate was ignored.
    //    STATUS_SUCCESS                    The Candidate object was added to the tree; Existing was ignored.
    //    STATUS_INSUFFICIENT_RESOURCES     The operation failed due to insufficient memory.
    //
    NTSTATUS    FindOrInsert(
        __in        T& Candidate,
        __in        KeyType& KeyVal,
        __out       T& Existing
        )
    {
        AvlNode     keyNode(KeyVal);
        BOOLEAN     newElement;
        
        PVOID p = RtlInsertElementGenericTableAvl(&_AvlTable, &keyNode, sizeof(keyNode), &newElement);
        if (!p)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (newElement)
        {
            //
            // The RTL AVL table has allocated a buffer and memcopies the user data
            // into the buffer. 
            //       
            // Use the placement new operator to contruct an AvlNode object 
            // on the buffer.
            //
            
            new(p) AvlNode(KeyVal, Candidate);
            return STATUS_SUCCESS;
        }

        AvlNode* existingNode = reinterpret_cast<AvlNode*>(p);
        Existing = existingNode->Object;
        return STATUS_OBJECT_NAME_EXISTS;
    }

    //
    // Remove
    //
    // Removes an entry from the AVL tree.
    //
    // Parameters:
    //   KeyVal           The key to the item to be removed.
    //
    // Return value:
    //   TRUE if the item was found and removed
    //   FALSE if the specified item was not in the tree
    //
    BOOLEAN Remove(
        __in const KeyType& KeyVal
        )
    {
        AvlNode     keyNode(KeyVal);
        
        PVOID p = RtlLookupElementGenericTableAvl(&_AvlTable, &keyNode);
        if (!p)
        {
            return FALSE;
        }

        AvlNode* node = reinterpret_cast<AvlNode*>(p);
        return DeleteNode(node);
    }

    //
    // Extract
    //
    // Retrieves an item and removes it from the tree. The specified object must have assignment semantics.
    //
    // Parameters:
    //   KeyVal         The key to the item to be extracted.
    //   Object         Receives the object, if it was found.
    //
    // Return value:
    //   TRUE if the item was removed and Object was assigned.
    //   FALSE if the item was not found.
    //
    BOOLEAN Extract(
        __in  KeyType& KeyVal,
        __out T& Object
        )
    {
        AvlNode     keyNode(KeyVal);
        
        PVOID p = RtlLookupElementGenericTableAvl(&_AvlTable, &keyNode);
        if (!p)
        {
            return FALSE;
        }

        AvlNode* node = reinterpret_cast<AvlNode*>(p);
        Object = node->Object;

        return DeleteNode(node);
    }

    //
    // RemoveAll
    //
    // Empties the tree.
    //
    
    VOID RemoveAll()
    {
        PVOID p;
        for (;;) 
        {
            p = RtlEnumerateGenericTableAvl(&_AvlTable, TRUE);
            if (!p) 
            {
                break;
            }
        
            AvlNode* node = reinterpret_cast<AvlNode*>(p);
            DeleteNode(node);
        }
    }

    // Find
    //
    // Retrieves an interator starting at a specific element. The first item in the iterator
    // will be the matching element, but the user may continue to iterate through the tree.
    //
    // Parameters:
    //   KeyVal           The key to the item to be searched.
    //
    Iterator Find(
        __in KeyType& KeyVal
        )
    {
        //
        // Find the node. If the specified key does not exist,
        // still return an Iterator that occupies a slot.
        // We maintain the invariance that every Iterator object occupies a slot.
        //

        AvlNode     keyNode(KeyVal);                
        ULONG       nextFlag = FALSE;
        PVOID       restartKey = NULL;
        ULONG       deleteCount = 0;
        
        PVOID p = RtlEnumerateGenericTableLikeADirectory(&_AvlTable, NULL, NULL, 
                    nextFlag, &restartKey, &deleteCount, &keyNode);

        //
        // If the specified key cannot be found, RtlEnumerateGenericTableLikeADirectory()
        // returns the next element in the tree.
        // So verify the return value is correct here.
        //
        
        AvlNode* node = reinterpret_cast<AvlNode*>(p);   
        if (node && _ComparisonFunction(KeyVal, *(static_cast<KeyType*>(node))) != 0)
        {
            node = NULL;
            restartKey = NULL;
            deleteCount = 0;
        }
        
        //
        // Allocate an iterator for the search result.
        //
        
        for (ULONG i = 0; i < MaxIterators; ++i)
        {
            if (_Iterators[i].Free)
            {
                _Iterators[i].Free = FALSE;

                _Iterators[i].Zero();                
                _Iterators[i].Node = node;
                _Iterators[i].DeleteCount = deleteCount;
                _Iterators[i].RestartKey = restartKey;                
                
                return Iterator(this, i, FALSE);
            }
        }        

        // Running out of iterators is a usage bug.
        ASSERT(FALSE);
        return Iterator();
    }

    // FindInRange
    //
    // Retrieves the element which holds the range being searched for
    //
    // Parameters:
    //   KeyVal           The key to the item to be searched.
    //
    BOOLEAN FindInRange(
        __inout KeyType& KeyVal,
        __out T& Object
        )
    {
        //
        // Find the node. If the specified key does not exist,
        // still return an Iterator that occupies a slot.
        // We maintain the invariance that every Iterator object occupies a slot.
        //

        AvlNode     keyNode(KeyVal);                
        ULONG       nextFlag = FALSE;
        PVOID       restartKey = NULL;
        ULONG       deleteCount = 0;
        
        PVOID p = RtlEnumerateGenericTableLikeADirectory(&_AvlTable, NULL, NULL, 
                    nextFlag, &restartKey, &deleteCount, &keyNode);

        //
        // If the specified key cannot be found, RtlEnumerateGenericTableLikeADirectory()
        // returns the next element in the tree.
        // So verify the return value is correct here.
        //
        
        AvlNode* node = reinterpret_cast<AvlNode*>(p);   
        if (node && _ComparisonFunction(KeyVal, *(static_cast<KeyType*>(node))) != 0)
        {
            node = NULL;
        }

		if (node)
		{
			KeyVal = *(static_cast<KeyType*>(node));
			Object = node->Object;
			return TRUE;
		}
		
		return FALSE;
    }

	
    // Find
    //
    // An overload that retrieves a single instance, if it is present.
    //
    // Parameters:
    //   KeyVal           The key to the item to be searched.
    //   Object           Receives the object, if it was found.  Object must have asssigment semantics.
    //
    BOOLEAN Find(
        __in  KeyType& KeyVal,
        __out T& Object
        )
    {
        AvlNode     keyNode(KeyVal);
        
        PVOID p = RtlLookupElementGenericTableAvl(&_AvlTable, &keyNode);
        if (!p)
        {
            return FALSE;
        }

        AvlNode* node = reinterpret_cast<AvlNode*>(p);
        Object = node->Object;
        return TRUE;
    }

    // GetIterator
    //
    // Retrieves an iterator.  The iterator is positioned at the First element of the tree.
    // If the maximum number of iterators has been exceeded or the tree is empty, the Iterator will be internally
    // Invalid, so the caller should check the iterator with IsValid() before retrieving the next item.
    //
    // Return value:
    //   Iterator    An iterator for the current tree.
    //
    Iterator GetIterator()
    {
        for (ULONG i = 0; i < MaxIterators; ++i)
        {
            if (_Iterators[i].Free)
            {
                _Iterators[i].Free = FALSE;
                _Iterators[i].Zero();      

                return Iterator(this, i, TRUE);
            }
        }

        // Running out of iterators is a usage bug.
        ASSERT(FALSE);        
        return Iterator();
    }

private:    
    
    //
    // Hide the default constructor.
    // A KAvlTree must have a key comparison function.
    //
    KAvlTree();
    
    //
    // Internal node stored in the AVL tree user buffer.
    //
    
    struct AvlNode : public KeyType
    {
        AvlNode(__in const KeyType& KeyVal) :
            KeyType(KeyVal)
        {
        }
        
        AvlNode(__in const KeyType& KeyVal, __in const T& Src) :
            KeyType(KeyVal), Object(Src)
        {
        }
            
        T Object;            
    };   

    static 
    __drv_sameIRQL
    __drv_functionClass(RTL_AVL_COMPARE_ROUTINE)        
    RTL_GENERIC_COMPARE_RESULTS
    AvlCompareRoutine(
        __in PRTL_AVL_TABLE     Table,
        __in PVOID              First,
        __in PVOID              Second
        )
    {
        KAvlTreeType* avlTree = reinterpret_cast<KAvlTreeType*>(Table->TableContext);
            
        KeyType* firstKey = reinterpret_cast<KeyType*>(First);
        KeyType* secondKey = reinterpret_cast<KeyType*>(Second);        
                    
        LONG result = avlTree->_ComparisonFunction(*firstKey, *secondKey);
        if (result < 0)
        {
            return GenericLessThan;
        }
        else if (result > 0)
        {
            return GenericGreaterThan;
        }
        else
        {
            return GenericEqual;
        }
    }

    static 
    __drv_sameIRQL
    __drv_functionClass(RTL_AVL_ALLOCATE_ROUTINE)
    __drv_allocatesMem(Mem)        
    PVOID
    AvlAllocRoutine(
        __in PRTL_AVL_TABLE     Table,
        __in CLONG              Bytes
        )        
    {
        KAvlTree* t = (KAvlTree*) Table->TableContext;
        return _newArray<UCHAR>(t->_AllocationTag, t->_Allocator, Bytes);
    }
    
    static 
    __drv_sameIRQL
    __drv_functionClass(RTL_AVL_FREE_ROUTINE)        
    VOID
    AvlFreeRoutine(
        __in PRTL_AVL_TABLE     Table,
        __in __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
        )        
    {
        UNREFERENCED_PARAMETER(Table);   
        _deleteArray((PUCHAR)Buffer);
    }

    BOOLEAN DeleteNode(
        __in AvlNode* NodeToDelete
        )
    {
        //
        // Invalidate any iterator that points to the same node.
        //
        
        for (ULONG i = 0; i < MaxIterators; ++i)
        {
            if (_Iterators[i].Node == NodeToDelete)
            {
                _Iterators[i].Zero();
            }
        }
        
        //
        // The AvlNode object is constructed using a placement new().
        // We need to explicitely call its destructor.
        //
    
        NodeToDelete->~AvlNode();

        //
        // Delete the RTL AVL table node structure, which includes
        // both _RTL_BALANCED_LINKS header and the buffer to hold 
        // the AvlNode object.
        //
        
        return RtlDeleteElementGenericTableAvl(&_AvlTable, NodeToDelete);         
    }    

    //
    // Internal version of an iterator.
    // It contains state of its current location so that moving to the next node is cheap.
    //
    struct AvlIterator
    {
        AvlIterator() : Free(TRUE)
        {
            Zero();
        }

        VOID Zero()
        {
            Node = NULL;
            RestartKey = NULL;
            DeleteCount = 0;
        }

        AvlNode*    Node;        
        PVOID       RestartKey;     // For resuming from RtlEnumerateGenericTableLikeADirectory()
        ULONG       DeleteCount;    // For resuming from RtlEnumerateGenericTableLikeADirectory()
        BOOLEAN     Free;      
    };
    
    AvlIterator         _Iterators[MaxIterators];
    RTL_AVL_TABLE       _AvlTable; 
    KeyComparisonFunc   _ComparisonFunction;
    KAllocator&         _Allocator;
    ULONG               _AllocationTag;
};
