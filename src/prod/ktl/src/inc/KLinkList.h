/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KLinkList.h

    Description:
      Kernel Template Library (KTL): General Raw Linked List Definitions

    History:
      richhas          23-Sep-2010         Initial version.

--*/
#pragma once


// BUG, richhas, xxxxxx, replace use cases with KNode and KNodeList at some point
class LinkListEntry 
{
    K_DENY_COPY(LinkListEntry);

public:
    inline 
    LinkListEntry();

    inline LinkListEntry& 
    InsertAfter(LinkListEntry& After);

    inline LinkListEntry& 
    Remove();

    inline BOOLEAN 
    IsLinked();

    inline LinkListEntry& 
    GetNext();

    inline LinkListEntry& 
    GetPrevious();

private:
    inline VOID 
    InitLinks();

protected:
    LinkListEntry* _Next;
    LinkListEntry* _Prev;
};

class LinkListHeader : private LinkListEntry 
{
    K_DENY_COPY(LinkListHeader);

public:
    inline 
    LinkListHeader();

    operator LinkListEntry const *() { return this; }

    inline BOOLEAN 
    IsEmpty();

    inline BOOLEAN 
    IsEndOfList(LinkListEntry& Entry);

    inline LinkListEntry& 
    InsertTop(LinkListEntry& NewEntry);

    inline LinkListEntry& 
    InsertBottom(LinkListEntry& NewEntry);

    inline LinkListEntry& 
    GetTop();

    inline LinkListEntry& 
    GetBottom();
};

//** LinkList inline implementation
inline VOID 
LinkListEntry::InitLinks() 
{
    _Next = this;
    _Prev = this;
}

inline 
LinkListEntry::LinkListEntry() 
{ 
    InitLinks(); 
}

inline LinkListEntry& 
LinkListEntry::InsertAfter(LinkListEntry& After)
{
    KAssert((_Next == _Prev) && _Next == this);
    _Next = After._Next;
    After._Next = this;
    _Prev = _Next->_Prev;
    _Next->_Prev = this;
    return *this;
}

inline LinkListEntry& 
LinkListEntry::Remove() 
{
    _Prev->_Next = _Next;
    _Next->_Prev = _Prev;
    InitLinks();
    return *this;
}

inline BOOLEAN 
LinkListEntry::IsLinked() 
{
    return _Prev != this;
}

inline LinkListEntry& 
LinkListEntry::GetNext() 
{
    return *_Next;
}

inline LinkListEntry& 
LinkListEntry::GetPrevious() 
{
    return *_Prev;
}

//** ListListHeader inline implementation
inline 
LinkListHeader::LinkListHeader() 
{
}

inline BOOLEAN 
LinkListHeader::IsEmpty() 
{
    return !IsLinked();
}

inline BOOLEAN 
LinkListHeader::IsEndOfList(LinkListEntry& Entry) 
{
    return this == &Entry;
}

inline LinkListEntry& 
LinkListHeader::InsertTop(LinkListEntry& NewEntry) 
{
    NewEntry.InsertAfter(*this);
    return NewEntry;
}

inline LinkListEntry& 
LinkListHeader::InsertBottom(LinkListEntry& NewEntry) 
{
    NewEntry.InsertAfter(*_Prev);
    return NewEntry;
}

inline LinkListEntry& 
LinkListHeader::GetTop() 
{
    KAssert(!IsEmpty());
    return *_Next;
}

inline LinkListEntry& 
LinkListHeader::GetBottom() 
{
    KAssert(!IsEmpty());
    return *_Prev;
}


