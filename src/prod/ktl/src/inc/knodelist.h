/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    knodelist

    Description:
      Kernel Tempate Library (KTL): KNodeList

      A templatized wrapper for LIST_ENTRY objects which
      have NOFAIL properties.

    History:
      raymcc          16-Aug-2010         Initial version.
      raymcc          20-Oct-2010         Revision for named links

    Typical Use:

    KNodeList<MyType> TheList(FIELD_OFFSET(MyType, FieldName));

    ...where FieldName is the name of the LIST_ENTRY to be used.

    If the LIST_ENTRY is a private member of a class, then implement
    a static const int for the class that returns the offset:

    class Sample
    {
       private:
          LIST_ENTRY Link1;
       public:
          Sample() {}
          static const int OffsetLink1;
    };
    const int Sample::OffsetLink1 = FIELD_OFFSET(Sample, Link1);

    This value can be used in the constructor.
--*/

#pragma once


//
// KListEntry
//
// Simple derivative of LIST_ENTRY which
// initializes the fields to null an construct-time.
//
struct KListEntry : public LIST_ENTRY
{
    KListEntry() { Flink = Blink = nullptr; }
    ~KListEntry() { KFatal((Flink == nullptr) && (Blink == nullptr)); }
};

#define  __KNodeList_ComputeT(Le)      ((T*)(PUCHAR(Le) - _LinkOffset))
#define  __KNodeList_ComputeLe(T)      (PLIST_ENTRY(PUCHAR(T)+_LinkOffset))

#define  __KNodeList_WipeLink(T) \
      __KNodeList_ComputeLe(T)->Flink = __KNodeList_ComputeLe(T)->Blink = nullptr


#define  __KNodeList_ValidateLink(T) \
      KFatal((__KNodeList_ComputeLe(T)->Flink == nullptr) && \
              (__KNodeList_ComputeLe(T)->Blink == nullptr))


//
// KNodeList
//
// Implements a NOFAIL list in which the links
// are already present in the object being included on the list.
// The mechanism simply referneces those links to add/remove
// the object from the list.
//
//
template <class T>
class KNodeList
{
public:
    // KNodeList
    //
    // Constructor
    //
    // Parameters:
    //  LinkOffset      The offset into the object where the LIST_ENTRY
    //                  object is located for this particular list. Use the
    //                  standard Windows FIELD_OFFSET macro for this.
    //
    KNodeList(ULONG LinkOffset)
    {
        _LinkOffset = LinkOffset;
        InitializeListHead(&_Head);
        _Count = 0;
    }

    // KNodeList move constructor - move the content of FromList to the current instance
    //
    //  Parameters: FromList - source list - empty after the move
    //
    //  Typical usage:
    //
    //      KNodeList<X> srcList;
    //      KNodeList<X> destList(Ktl::Move(srcList));
    //
    KNodeList(KNodeList<T>&& FromList)
    {
        _LinkOffset = FromList._LinkOffset;
        _Count = 0;
        InitializeListHead(&_Head);
        AppendTail(FromList);
    }

    ULONG GetLinkOffset()
    {
        return _LinkOffset;
    }

    // InsertHead
    //
    // Inserts the object to become the new head of the list.
    //
    // Parameters:
    //    ObjectToInsert         Pointer to the object to be inserted.
    //
    void InsertHead(
        __in T* ObjectToInsert
        )
    {
        __KNodeList_ValidateLink(ObjectToInsert);
        InsertHeadList(&_Head, PLIST_ENTRY(PUCHAR(ObjectToInsert)+_LinkOffset));
        _Count++;
    }

    // AppendTail
    //
    // Inserts the object to become the new tail.
    //
    // Parameters:
    //    ObjectToInsert         Pointer to the object to be inserted.
    //
    void AppendTail(
        __in T* ObjectToInsert
        )
    {
        __KNodeList_ValidateLink(ObjectToInsert);
        InsertTailList(&_Head, PLIST_ENTRY(PUCHAR(ObjectToInsert)+_LinkOffset));
        _Count++;
    }

    // InsertAfter
    //
    // Inserts the new node after (toward the tail) the specified target object.
    //
    // Parameters:
    //    ObjectToInsert        Pointer to the object to insert.
    //    Target                Pointer to the object acting as the cursor.
    //
    void InsertAfter(
        __in T* ObjectToInsert,
        __in T* Target
        )
    {
        PLIST_ENTRY TargLe = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        PLIST_ENTRY NewLe = PLIST_ENTRY(PUCHAR(ObjectToInsert)+_LinkOffset);

        __KNodeList_ValidateLink(ObjectToInsert);

        NewLe->Blink = TargLe;
        NewLe->Flink = TargLe->Flink;
        NewLe->Flink->Blink = NewLe;
        TargLe->Flink = NewLe;

        _Count++;
    }

    // InsertBefore
    //
    // Inserts the new node before (toward the head) the specified target object.
    //
    // Parameters:
    //    ObjectToInsert        Pointer to the object to insert.
    //    Target                Pointer to the object acting as the cursor.
    //
    void InsertBefore(
        __in T* ObjectToInsert,
        __in T* Target
        )
    {
        PLIST_ENTRY TargLe = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        PLIST_ENTRY NewLe = PLIST_ENTRY(PUCHAR(ObjectToInsert)+_LinkOffset);

        __KNodeList_ValidateLink(ObjectToInsert);

        NewLe->Flink = TargLe;
        NewLe->Blink = TargLe->Blink;
        NewLe->Blink->Flink = NewLe;
        TargLe->Blink = NewLe;

        _Count++;
    }

    //  PeekHead
    //
    //  Return value:
    //     Returns the first element in the list or NULL for an empty list.
    //
    T* PeekHead()
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        LIST_ENTRY* Link = _Head.Flink;
        T* Result = (T*)(PUCHAR(Link) - _LinkOffset);
        return Result;
    }

    //  PeekTail
    //
    //  Return value:
    //     Returns the last element in the list or NULL for an empty list.
    //
    T* PeekTail()
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        LIST_ENTRY* Link = _Head.Blink;
        T* Result = (T*)(PUCHAR(Link) - _LinkOffset);
        return Result;
    }

    //  Count
    //
    //  Return value:
    //     The number of nodes in the list.
    //
    ULONG Count()
    {
        return _Count;
    }

    // IsEmpty
    //
    // Return value:
    //    TRUE if the list is empty
    //    FALSE if it the list is not empty
    //
    BOOLEAN IsEmpty()
    {
        return IsListEmpty(&_Head);
    }

    // IsOnList
    //
    // Return value:
    //    TRUE if the entry is on this list
    //    FALSE if the entry is not on this list.
    //
    BOOLEAN IsOnList(T *Target)
    {
        PLIST_ENTRY Le = __KNodeList_ComputeLe(Target);
        PLIST_ENTRY Next;

        if (Le->Flink == nullptr && Le->Blink == nullptr)
        {
            return(FALSE);
        }

        for (Next = _Head.Flink;  Next != Le && Next != &_Head; Next = Next->Flink)
        {

        }

        return( Next == Le );
    }

    // RemoveHead
    //
    // Removes the head node (first node).
    // Note that the node is not deallocated.
    //
    // Return value:
    //    A pointer to the 'former' head of the list
    //    which was just removed.
    //
    T* RemoveHead()
    {
        if (!IsEmpty())
        {
            T* FormerHead = PeekHead();
            RemoveHeadList(&_Head);
            _Count--;
            __KNodeList_WipeLink(FormerHead);
            return FormerHead;
        }
        else return NULL;
    }

    // RemoveTail
    //
    // Removes the tail node (last node).
    // Note that the node is not deallocate.
    //
    // Return value:
    //    A pointer to the former tail of the
    //    list which was just removed.
    //
    T* RemoveTail()
    {
        if (!IsEmpty())
        {
            T* FormerTail = PeekTail();
            RemoveTailList(&_Head);
            __KNodeList_WipeLink(FormerTail);
            _Count--;
            return FormerTail;
        }
        else return NULL;
    }

    // Remove
    //
    // Removes the specified object from the list.
    // Note that the node is not deallocated.
    // The argument is returned for consistency with KNodeListShared
    //
    T* Remove(T* Obj)
    {
        PLIST_ENTRY Le = __KNodeList_ComputeLe(Obj);

        if (Le->Blink == nullptr && Le->Flink == nullptr)
        {
            return(nullptr);

        } else {

            // KAssert(IsOnList(Obj));
            RemoveEntryList(PLIST_ENTRY(PUCHAR(Obj)+_LinkOffset));
            __KNodeList_WipeLink(Obj);
            _Count--;
            return(Obj);
        }
    }

    // Successor
    //
    // Returns the next element (toward the tail) in the list.
    //
    // Parameters:
    //  Target   The current element whose successor is required.
    //
    // Return value:
    //  T*       The next element, or NULL if end-of-list has been reached.
    //
    T* Successor(T* Target)
    {
        PLIST_ENTRY Link = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        Link = Link->Flink;
        if (Link == &_Head)
            return NULL;
        return (T*)(PUCHAR(Link)-_LinkOffset);
    }

    // Predecessor
    //
    // Returns the previous element (toward the head) in the list.
    //
    // Parameters:
    //  Target   The current element whose predecessor is required.
    //
    // Return value:
    //  T*       The next element, or NULL if begin-of-list has been reached.
    //
    T* Predecessor(T* Target)
    {
        PLIST_ENTRY Link = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        Link = Link->Blink;
        if (Link == &_Head)
            return NULL;
        return (T*)(PUCHAR(Link)-_LinkOffset);
    }

    //  Reset
    //
    //  Set the list to 'empty'.
    //  None of the nodes are deallocated.
    //
    void Reset()
    {
        //
        // Elements on the list need to be "removed" which resets their link pointer.
        //

        while(RemoveHead() != nullptr)
        {

        }

        KAssert(_Count == 0);
    }

    //  AppendTail
    //
    //  Take the entire list and append it to this one, leaving the given list empty.
    //
    void AppendTail(
        __inout KNodeList<T>& Source
        )
    {
        KFatal(Source._LinkOffset == _LinkOffset);

        if (!Source._Count) {
            return;
        }

        if (_Count) {

            _Head.Blink->Flink = Source._Head.Flink;
            Source._Head.Flink->Blink = _Head.Blink;
            Source._Head.Blink->Flink = &_Head;
            _Head.Blink = Source._Head.Blink;

        } else {

            _Head.Flink = Source._Head.Flink;
            _Head.Blink = Source._Head.Blink;
            _Head.Flink->Blink = &_Head;
            _Head.Blink->Flink = &_Head;

        }

        _Count += Source._Count;

        Source._Count = 0;
        InitializeListHead(&Source._Head);
    }

    //
    // Destructor removes all elements on the list.
    //
    ~KNodeList() {

        Reset();

        //
        // Need to zero out the list head so the
        // KListEntry destructor does not assert.
        //

        _Head.Flink = _Head.Blink = nullptr;
    }

protected:
    // KNodeList
    //
    // Constructor support for derivations
    //
    KNodeList()
    {
        _LinkOffset = MAXULONG;
        InitializeListHead(&_Head);
        _Count = 0;
    }

protected:
    KListEntry  _Head;
    ULONG       _Count;
    ULONG       _LinkOffset;
};

#ifndef KTL_CORE_LIB
//
// KNodeListShared
//
// Implements a NOFAIL list in which the links
// are already present in the object being included on the list.
// The mechanism simply referneces those links to add/remove
// the object from the list.  Object are referenced and dereferenced
// as they are added and removed from the list.  Objects must inherit
// from KShared.
//
template <class T>
class KNodeListShared
{
public:
    // KNodeList
    //
    // Constructor
    //
    // Parameters:
    //  LinkOffset      The offset into the object where the LIST_ENTRY
    //                  object is located for this particular list. Use the
    //                  standard Windows FIELD_OFFSET macro for this.
    //
    KNodeListShared(ULONG LinkOffset)
    {
        _LinkOffset = LinkOffset;
        InitializeListHead(&_Head);
        _Count = 0;
    }

    // InsertHead
    //
    // Inserts the object to become the new head of the list.
    //
    // Parameters:
    //    ObjectToInsert         Pointer to the object to be inserted.
    //
    void InsertHead(
        __in KSharedPtr<T> ObjectToInsert
        )
    {
        __KNodeList_ValidateLink(ObjectToInsert.RawPtr());
        InsertHeadList(&_Head, PLIST_ENTRY(PUCHAR(ObjectToInsert.Detach())+_LinkOffset));
        _Count++;
    }

    // AppendTail
    //
    // Inserts the object to become the new tail.
    //
    // Parameters:
    //    ObjectToInsert         Pointer to the object to be inserted.
    //
    void AppendTail(
        __in KSharedPtr<T> ObjectToInsert
        )
    {
        __KNodeList_ValidateLink(ObjectToInsert.RawPtr());
        InsertTailList(&_Head, PLIST_ENTRY(PUCHAR(ObjectToInsert.Detach())+_LinkOffset));
        _Count++;
    }

    // InsertAfter
    //
    // Inserts the new node after (toward the tail) the specified target object.
    //
    // Parameters:
    //    ObjectToInsert        Pointer to the object to insert.
    //    Target                Pointer to the object acting as the cursor.
    //
    void InsertAfter(
        __in KSharedPtr<T> ObjectToInsert,
        __in T* Target
        )
    {

        __KNodeList_ValidateLink(ObjectToInsert.RawPtr());

        PLIST_ENTRY TargLe = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        PLIST_ENTRY NewLe = PLIST_ENTRY(PUCHAR(ObjectToInsert.Detach())+_LinkOffset);

        NewLe->Blink = TargLe;
        NewLe->Flink = TargLe->Flink;
        NewLe->Flink->Blink = NewLe;
        TargLe->Flink = NewLe;

        _Count++;
    }

    // InsertBefore
    //
    // Inserts the new node before (toward the head) the specified target object.
    //
    // Parameters:
    //    ObjectToInsert        Pointer to the object to insert.
    //    Target                Pointer to the object acting as the cursor.
    //
    void InsertBefore(
        __in KSharedPtr<T> ObjectToInsert,
        __in T* Target
        )
    {
        __KNodeList_ValidateLink(ObjectToInsert.RawPtr());

        PLIST_ENTRY TargLe = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        PLIST_ENTRY NewLe = PLIST_ENTRY(PUCHAR(ObjectToInsert.Detach())+_LinkOffset);

        NewLe->Flink = TargLe;
        NewLe->Blink = TargLe->Blink;
        NewLe->Blink->Flink = NewLe;
        TargLe->Blink = NewLe;

        _Count++;
    }

    //  PeekHead
    //
    //  Return value:
    //     Returns the first element in the list or NULL for an empty list.
    //
    T* PeekHead()
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        LIST_ENTRY* Link = _Head.Flink;
        T* Result = (T*)(PUCHAR(Link) - _LinkOffset);
        return Result;
    }

    //  PeekTail
    //
    //  Return value:
    //     Returns the last element in the list or NULL for an empty list.
    //
    T* PeekTail()
    {
        if (IsEmpty())
        {
            return nullptr;
        }
        LIST_ENTRY* Link = _Head.Blink;
        T* Result = (T*)(PUCHAR(Link) - _LinkOffset);
        return Result;
    }

    //  Count
    //
    //  Return value:
    //     The number of nodes in the list.
    //
    ULONG Count()
    {
        return _Count;
    }

    // IsEmpty
    //
    // Return value:
    //    TRUE if the list is empty
    //    FALSE if it the list is not empty
    //
    BOOLEAN IsEmpty()
    {
        return IsListEmpty(&_Head);
    }


    // IsOnList
    //
    // Return value:
    //    TRUE if the entry is on this list
    //    FALSE if the entry is not on this list.
    //
    BOOLEAN IsOnList(T *Target)
    {
        PLIST_ENTRY Le = __KNodeList_ComputeLe(Target);
        PLIST_ENTRY Next;

        if (Le->Flink == nullptr && Le->Blink == nullptr)
        {
            return(FALSE);
        }

        for (Next = _Head.Flink;  Next != Le && Next != &_Head; Next = Next->Flink)
        {
        }

        return( Next == Le );
    }

    // RemoveHead
    //
    // Removes the head node (first node).
    // Note that the node is deallocated.
    //
    // Return TRUE if a element was removed.
    //

    KSharedPtr<T> RemoveHead()
    {
        if (!IsEmpty())
        {
            KSharedPtr<T> FormerHead;
            FormerHead.Attach(PeekHead());
            RemoveHeadList(&_Head);
            _Count--;
            __KNodeList_WipeLink(FormerHead.RawPtr());
            return FormerHead;
        } else {
            return nullptr;
        }
    }

    // REMOVETAIL
    //
    // Removes the tail node (last node).
    //
    // Returns a shared pointer to the object or
    // null if the list is empty
    //

    KSharedPtr<T> RemoveTail()
    {
        if (!IsEmpty())
        {
            KSharedPtr<T> FormerTail;
            FormerTail.Attach(PeekTail());
            RemoveTailList(&_Head);
            _Count--;
            __KNodeList_WipeLink(FormerTail.RawPtr());
            return FormerTail;
        }
        else return nullptr;
    }

    // Remove
    //
    // Removes the specified object from the list.
    // Note that the node is deallocated.
    //

    KSharedPtr<T> Remove(T* Obj)
    {
        PLIST_ENTRY Le = __KNodeList_ComputeLe(Obj);

        if (Le->Blink == nullptr && Le->Flink == nullptr)
        {
            return(nullptr);

        } else {

            KSharedPtr<T> ReturnPtr;

            // KAssert(IsOnList(Obj));
            RemoveEntryList(PLIST_ENTRY(PUCHAR(Obj)+_LinkOffset));
            ReturnPtr.Attach(Obj);
            __KNodeList_WipeLink(Obj);
            _Count--;
            return(ReturnPtr);
        }
    }


    // Successor
    //
    // Returns the next element (toward the tail) in the list.
    //
    // Parameters:
    //  Target   The current element whose successor is required.
    //
    // Return value:
    //  T*       The next element, or NULL if end-of-list has been reached.
    //
    T* Successor(T* Target)
    {
        PLIST_ENTRY Link = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        Link = Link->Flink;
        if (Link == &_Head)
            return NULL;
        return (T*)(PUCHAR(Link)-_LinkOffset);
    }

    // Predecessor
    //
    // Returns the previous element (toward the head) in the list.
    //
    // Parameters:
    //  Target   The current element whose predecessor is required.
    //
    // Return value:
    //  T*       The next element, or NULL if begin-of-list has been reached.
    //
    T* Predecessor(T* Target)
    {
        PLIST_ENTRY Link = PLIST_ENTRY(PUCHAR(Target)+_LinkOffset);
        Link = Link->Blink;
        if (Link == &_Head)
            return NULL;
        return (T*)(PUCHAR(Link)-_LinkOffset);
    }



    //  Reset
    //
    //  Set the list to 'empty'.
    //  The nodes are released or dereferenced.
    //
    void Reset()
    {
        //
        // Remove all the elements from the list. This will also release each element.
        //

        while (RemoveHead() != nullptr)
        {

        }

        KAssert(_Count == 0);
    }

    //  AppendTail
    //
    //  Take the entire list and append it to this one, leaving the given list empty.
    //
    void AppendTail(
        __inout KNodeListShared<T>& Source
        )
    {
        KFatal(Source._LinkOffset == _LinkOffset);

        if (!Source._Count) {
            return;
        }

        if (_Count) {

            _Head.Blink->Flink = Source._Head.Flink;
            Source._Head.Flink->Blink = _Head.Blink;
            Source._Head.Blink->Flink = &_Head;
            _Head.Blink = Source._Head.Blink;

        } else {

            _Head.Flink = Source._Head.Flink;
            _Head.Blink = Source._Head.Blink;
            _Head.Flink->Blink = &_Head;
            _Head.Blink->Flink = &_Head;

        }

        _Count += Source._Count;

        Source._Count = 0;
        InitializeListHead(&Source._Head);
    }

    //
    // Destructor removes and releases all elments on the list.
    //

    ~KNodeListShared() {

        Reset();

        //
        // Need to zero out the list head so the
        // KListEntry destructor does not assert.
        //

        _Head.Flink = _Head.Blink = nullptr;
    }

private:
    KListEntry  _Head;
    ULONG       _Count;
    ULONG       _LinkOffset;
};

#endif // !KTL_CORE_LIB

