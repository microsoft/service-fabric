/*++

Module Name:

    knodetable.cpp

Abstract:

    This file contains the non-template parts of the the user mode and kernel mode implementations of a KNodeTable object.

Author:

    Norbert P. Kusters (norbertk) 29-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>


KTableEntry::~KTableEntry(
    )
{
    KFatal(!_SubtreeHeight);
}

KTableEntry::KTableEntry(
    )

/*++

Routine Description:

    This is a simple initialization of the balanced links so that we can detect the double insertion of an element.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _SubtreeHeight = 0;
    _Reserved = 0;
}

BOOLEAN
KTableEntry::IsInserted(
    )

/*++

Routine Description:

    This routine returns whether or not this table entry has been inserted into a KNodeTable.

Arguments:

    None.

Return Value:

    FALSE   - This node entry has not been inserted.

    TRUE    - This node entry has been inserted.

--*/

{
    return _SubtreeHeight ? TRUE : FALSE;
}

KNodeTableBase::~KNodeTableBase(
    )
{
}

KNodeTableBase::KNodeTableBase(
    __in ULONG LinksOffset,
    __in BaseCompareFunction Compare,
    __in_opt VOID* BaseAddress,
    __in_opt ULONGLONG RootOffset,
    __in_opt ULONG Count
    )

/*++

Routine Description:

    This is the constructor for a KNodeTableBase.  The KNodeTableBase just initializes to an empty state.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _BaseAddress = BaseAddress;
    _Root = TableEntryFromOffset(RootOffset);
    _Count = Count;
    _LinksOffset = LinksOffset;
    _Compare = Compare;
}

VOID
KNodeTableBase::SetRoot(
    __in VOID* BaseAddress,
    __in ULONGLONG RootOffset,
    __in ULONG Count
    )

/*++

Routine Description:

    This routine sets the current state of the entire tree relative to a new piece of memory with an embedded avl tree.

Arguments:

    BaseAddress - Supplies the base address.

    RootOffset  - Supplies the offset of the root of the table relative to the base address.

    Count       - Supplies the number of elements in the table.

Return Value:

    None.

--*/

{
    _BaseAddress = BaseAddress;
    _Root = TableEntryFromOffset(RootOffset);
    _Count = Count;

    //
    // The root is not null if-and-only-if the count is not 0.
    //

    KFatal((_Root && _Count) || (!_Root && !_Count));
}

KTableEntry*
KNodeTableBase::Lookup(
    __in const VOID* SearchRecord
    )

/*++

Routine Description:

    This routine searches for an element in the table that is equal to the given search key element.

Arguments:

    SearchRecord    - Supplies the search record.

Return Value:

    The element in the table that is equal to the one given or NULL.

--*/

{
    KTableEntry* n;
    LONG r;

    n = _Root;

    while (n) {

        r = Compare(SearchRecord, n);

        if (!r) {
            return n;
        }

        if (r < 0) {
            n = TableEntryFromOffset(n->_LeftOffset);
        } else {
            n = TableEntryFromOffset(n->_RightOffset);
        }
    }

    return NULL;
}

KTableEntry*
KNodeTableBase::LookupEqualOrNext(
    __in const VOID* SearchRecord
    )

/*++

Routine Description:

    This routine searches for an element in the table that is equal to the given search key element or else the element in the
    table that is the least element greater than the given search record.

Arguments:

    SearchRecord    - Supplies the search record.

Return Value:

    The element in the table that is equal to the one given, or the one that is the least one greater than the given element,
    or NULL.

--*/

{
    KTableEntry* last = NULL;
    KTableEntry* n = NULL;
    LONG r = 0;

    last = NULL;
    n = _Root;

    while (n) {

        r = Compare(SearchRecord, n);

        if (!r) {
            return n;
        }

        last = n;

        if (r < 0) {
            n = TableEntryFromOffset(n->_LeftOffset);
        } else {
            n = TableEntryFromOffset(n->_RightOffset);
        }
    }

    if (!last) {
        return NULL;
    }

    if (r > 0) {
        last = Next(last);
    }

    return last;
}

KTableEntry*
KNodeTableBase::LookupEqualOrPrevious(
    __in const VOID* SearchRecord
    )

/*++

Routine Description:

    This routine searches for an element in the table that is equal to the given search key element or else the element in the
    table that is the greatest element less than the given search record.

Arguments:

    SearchRecord    - Supplies the search record.

Return Value:

    The element in the table that is equal to the one given, or the one that is the greatest one less than the given element,
    or NULL.

--*/

{
    KTableEntry* last = NULL;
    KTableEntry* n = NULL;
    LONG r = 0;

    last = NULL;
    n = _Root;

    while (n) {

        r = Compare(SearchRecord, n);

        if (!r) {
            return n;
        }

        last = n;

        if (r < 0) {
            n = TableEntryFromOffset(n->_LeftOffset);
        } else {
            n = TableEntryFromOffset(n->_RightOffset);
        }
    }

    if (!last) {
        return NULL;
    }

    if (r < 0) {
        last = Previous(last);
    }

    return last;
}

BOOLEAN
KNodeTableBase::Insert(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine will insert the given element into the tree provided that there isn't currently an element equal to this one.
    If the table entry given is already being used in another table, this routine will assert.

Arguments:

    TableEntry  - Supplies the table entry to insert into the table.

Return Value:

    FALSE   - The table entry was not inserted because there is already a table entry in the table with a key that is equal
                to this one.

    TRUE    - The table entry was inserted.

--*/

{
    KTableEntry* p;
    KTableEntry* n;
    LONG r;

    //
    // Check to make sure that this node is not already in another table.
    //

    KFatal(!TableEntry->_SubtreeHeight);

    //
    // Initialize the link entries to default values.
    //

    TableEntry->_ParentOffset = 0;
    TableEntry->_LeftOffset = 0;
    TableEntry->_RightOffset = 0;

    //
    // It the table is empty then just place this node at the root.
    //

    if (!_Root) {
        _Root = TableEntry;
        _Count = 1;
        TableEntry->_SubtreeHeight = 1;
        return TRUE;
    }

    //
    // Iterate down the tree.
    //

    p = NULL;
    n = _Root;

    for (;;) {

        r = Compare(TableEntry, n);

        if (!r) {

            //
            // There is already a duplicate of this table entry in the table.  Don't insert.
            //

            return FALSE;
        }

        if (r < 0) {

            //
            // The table entry we are looking for is less than this one.
            //

            p = n;
            n = TableEntryFromOffset(p->_LeftOffset);

            if (!n) {

                //
                // We have a blank spot here where the table entry fits, use it, balance later.
                //

                p->_LeftOffset = OffsetFromTableEntry(TableEntry);
                TableEntry->_ParentOffset = OffsetFromTableEntry(p);
                break;
            }

        } else {

            //
            // The table entry we are looking for is greater than this one.
            //

            p = n;
            n = TableEntryFromOffset(p->_RightOffset);

            if (!n) {

                //
                // We have a blank spot here where the table entry fits, use it, balance later.
                //

                p->_RightOffset = OffsetFromTableEntry(TableEntry);
                TableEntry->_ParentOffset = OffsetFromTableEntry(p);
                break;
            }
        }
    }

    _Count++;

    //
    // Balance.
    //

    BalanceUp(TableEntry);

    return TRUE;
}

VOID
KNodeTableBase::Remove(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine will remove the given table entry from the table.  Note there is no search here.  The given pointer is
    assumed to be in the table.  Use Lookup if you need to search first.

Arguments:

    TableEntry  - Supplies the table entry to remove.

Return Value:

    None.

--*/

{
    KTableEntry* p;
    KTableEntry* left;
    KTableEntry* right;
    KTableEntry* predecessor;
    KTableEntry* predecessorParent;

    //
    // If the subtree height field is 0, then the element has already been removed.
    //

    if (!TableEntry->_SubtreeHeight) {
        return;
    }

    //
    // Adjust the count.  Allow the element that will be removed to be reinserted later, possibly in another table.
    //

    TableEntry->_SubtreeHeight = 0;
    _Count--;

    //
    // Get the parent of the node being removed.
    //

    p = TableEntryFromOffset(TableEntry->_ParentOffset);

    //
    // If there isn't a right half, then there is only a left half.
    //

    if (!TableEntry->_RightOffset) {

        if (!p) {
            _Root = TableEntryFromOffset(TableEntry->_LeftOffset);
        } else if (TableEntryFromOffset(p->_LeftOffset) == TableEntry) {
            p->_LeftOffset = TableEntry->_LeftOffset;
        } else {
            p->_RightOffset = TableEntry->_LeftOffset;
        }

        if (TableEntry->_LeftOffset) {
            left = TableEntryFromOffset(TableEntry->_LeftOffset);
            left->_ParentOffset = OffsetFromTableEntry(p);
        }

        //
        // Balance.
        //

        BalanceUp(p);

        return;
    }

    //
    // If there isn't a left half, then there is only a right half.
    //

    if (!TableEntry->_LeftOffset) {

        if (!p) {
            _Root = TableEntryFromOffset(TableEntry->_RightOffset);
        } else if (p->_LeftOffset == OffsetFromTableEntry(TableEntry)) {
            p->_LeftOffset = TableEntry->_RightOffset;
        } else {
            p->_RightOffset = TableEntry->_RightOffset;
        }

        right = TableEntryFromOffset(TableEntry->_RightOffset);
        right->_ParentOffset = OffsetFromTableEntry(p);

        //
        // Balance.
        //

        BalanceUp(p);

        return;
    }

    //
    // There is both a left half and a right half.
    //

    //
    // Find the largest predecessor.
    //

    predecessor = TableEntryFromOffset(TableEntry->_LeftOffset);
    while (predecessor->_RightOffset) {
        predecessor = TableEntryFromOffset(predecessor->_RightOffset);
    }

    //
    // Take this largest predecessor and put it where the node that is being deleted was.
    //

    predecessorParent = TableEntryFromOffset(predecessor->_ParentOffset);
    if (TableEntryFromOffset(predecessorParent->_LeftOffset) == predecessor) {
        predecessorParent->_LeftOffset = predecessor->_LeftOffset;
    } else {
        predecessorParent->_RightOffset = predecessor->_LeftOffset;
    }

    if (predecessor->_LeftOffset) {
        left = TableEntryFromOffset(predecessor->_LeftOffset);
        left->_ParentOffset = OffsetFromTableEntry(predecessorParent);
    }

    predecessor->_ParentOffset = OffsetFromTableEntry(p);

    if (!p) {
        _Root = predecessor;
    } else if (TableEntryFromOffset(p->_LeftOffset) == TableEntry) {
        p->_LeftOffset = OffsetFromTableEntry(predecessor);
    } else {
        p->_RightOffset = OffsetFromTableEntry(predecessor);
    }

    predecessor->_LeftOffset = TableEntry->_LeftOffset;
    if (predecessor->_LeftOffset) {
        left = TableEntryFromOffset(predecessor->_LeftOffset);
        left->_ParentOffset = OffsetFromTableEntry(predecessor);
    }

    predecessor->_RightOffset = TableEntry->_RightOffset;
    if (predecessor->_RightOffset) {
        right = TableEntryFromOffset(predecessor->_RightOffset);
        right->_ParentOffset = OffsetFromTableEntry(predecessor);
    }

    //
    // Balance.
    //

    if (predecessorParent != TableEntry) {
        BalanceUp(predecessorParent);
    } else {
        BalanceUp(predecessor);
    }
}

KTableEntry*
KNodeTableBase::First(
    )

/*++

Routine Description:

    This routine returns the least element in the table.

Arguments:

    None.

Return Value:

    The least element or NULL.

--*/

{
    KTableEntry* n;

    if (!_Root) {
        return NULL;
    }

    n = _Root;
    while (n->_LeftOffset) {
        n = TableEntryFromOffset(n->_LeftOffset);
    }

    return n;
}

KTableEntry*
KNodeTableBase::Last(
    )

/*++

Routine Description:

    This routine returns the greatest element in the table.

Arguments:

    None.

Return Value:

    The least element or NULL.

--*/

{
    KTableEntry* n;

    if (!_Root) {
        return NULL;
    }

    n = _Root;
    while (n->_RightOffset) {
        n = TableEntryFromOffset(n->_RightOffset);
    }

    return n;
}

KTableEntry*
KNodeTableBase::Next(
    __in KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine returns the next greater element in the table.

Arguments:

    TableEntry  - Supplies an existing element of the table.

Return Value:

    The next element or NULL.

--*/

{
    KTableEntry* n;
    KTableEntry* p;

    KFatal(TableEntry->_SubtreeHeight);

    //
    // If the node has a right pointer then follow it to the right and then all the way to the left.
    //

    n = TableEntryFromOffset(TableEntry->_RightOffset);
    if (n) {
        while (n->_LeftOffset) {
            n = TableEntryFromOffset(n->_LeftOffset);
        }
        return n;
    }

    //
    // Otherwise follow the parent pointers until one is found that is off to the right.
    //

    n = TableEntry;

    for (;;) {

        p = TableEntryFromOffset(n->_ParentOffset);
        if (!p) {
            break;
        }

        if (TableEntryFromOffset(p->_LeftOffset) == n) {
            return p;
        }

        n = p;
    }

    return NULL;
}

KTableEntry*
KNodeTableBase::Previous(
    __in KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine returns the next greater element in the table.

Arguments:

    TableEntry  - Supplies an existing element of the table.

Return Value:

    The next element or NULL.

--*/

{
    KTableEntry* n;
    KTableEntry* p;

    KFatal(TableEntry->_SubtreeHeight);

    //
    // If the node has a left pointer then follow it to the left and then all the way to the right.
    //

    n = TableEntryFromOffset(TableEntry->_LeftOffset);
    if (n) {
        while (n->_RightOffset) {
            n = TableEntryFromOffset(n->_RightOffset);
        }
        return n;
    }

    //
    // Otherwise follow the parent pointers until one is found that is off to the left.
    //

    n = TableEntry;

    for (;;) {

        p = TableEntryFromOffset(n->_ParentOffset);
        if (!p) {
            break;
        }

        if (TableEntryFromOffset(p->_RightOffset) == n) {
            return p;
        }

        n = p;
    }

    return NULL;
}

ULONG
KNodeTableBase::Count(
    )

/*++

Routine Description:

    This method returns the number of elements in the table.

Arguments:

    None.

Return Value:

    The number of elements in the table.

--*/

{
    return _Count;
}

ULONGLONG
KNodeTableBase::QueryRootOffset(
    )

/*++

Routine Description:

    This routine returns the current root offset of the table so that it can be stored along with the memory comprising an
    entire table.

Arguments:

    None.

Return Value:

    The root offset of the table relative to the base address.

--*/

{
    return OffsetFromTableEntry(_Root);
}

KTableEntry*
KNodeTableBase::TableEntryFromOffset(
    __in ULONGLONG Offset
    )
{
    if (!Offset) {
        return NULL;
    }

    return (KTableEntry*) ((UCHAR*) _BaseAddress + Offset);
}

ULONGLONG
KNodeTableBase::OffsetFromTableEntry(
    __in KTableEntry* TableEntry
    )
{
    if (!TableEntry) {
        return 0;
    }

    return ((ULONGLONG) TableEntry) - ((ULONGLONG) _BaseAddress);
}

LONG
KNodeTableBase::Compare(
    __in const VOID* SearchRecord,
    __in KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine compares the given search record and table entry.

Arguments:

    SearchRecord    - Supplies the search record.

    TableEntry      - Supplies a table entry embedded within a record.

Return Value:

    <0  - 'SearchRecord' is less than 'TableEntry'.
    0   - 'SearchRecord' and 'TableEntry' are equal.
    >1  - 'SearchRecord' is greater than 'TableEntry'.

--*/

{
    return _Compare(SearchRecord, (VOID*) ((UCHAR*) TableEntry - _LinksOffset));
}

LONG
KNodeTableBase::Compare(
    __in KTableEntry* First,
    __in KTableEntry* Second
    )

/*++

Routine Description:

    This routine compares the given table entries

Arguments:

    First   - Supplies the first table entry.

    Second  - Supplies the second table entry.

Return Value:

    <0  - 'SearchRecord' is less than 'TableEntry'.
    0   - 'SearchRecord' and 'TableEntry' are equal.
    >1  - 'SearchRecord' is greater than 'TableEntry'.

--*/

{
    return _Compare((VOID*) ((UCHAR*) First - _LinksOffset), (VOID*) ((UCHAR*) Second - _LinksOffset));
}

VOID
KNodeTableBase::BalanceUp(
    __inout_opt KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine balances up the tree, starting at the given table entry.

Arguments:

    TableEntry  - Supplies the table entry.

Return Value:

    None.

--*/

{
    KTableEntry* n;

    n = TableEntry;
    while (n) {
        SetHeight(n);
        Rebalance(n);
        n = TableEntryFromOffset(n->_ParentOffset);
    }
}

VOID
KNodeTableBase::SetHeight(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine sets the height of the given table entry as the max of its two sub-trees plus 1.

Arguments:

    TableEntry  - Supplies the table entry to set the height of.

Return Value:

    None.

--*/

{
    LONG maxHeight;
    LONG rightHeight;

    maxHeight = TableEntry->_LeftOffset ? TableEntryFromOffset(TableEntry->_LeftOffset)->_SubtreeHeight : 0;
    rightHeight = TableEntry->_RightOffset ? TableEntryFromOffset(TableEntry->_RightOffset)->_SubtreeHeight : 0;
    if (maxHeight < rightHeight) {
        maxHeight = rightHeight;
    }

    TableEntry->_SubtreeHeight = maxHeight + 1;
}

VOID
KNodeTableBase::Rebalance(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    The given node is out of balance, meaning, the difference in its left and right
    halves is 2 whereas it should be no more than 1.  This routine rebalances by doing
    the necessary rotation.

Arguments:

    TableEntry  - Supplies the out-of-balance node to fix up.

Return Value:

    None.

--*/

{
    LONG diff;

    //
    // Rotate if the balance is off by more than 1.
    //

    diff = QueryBalance(TableEntry);

    if (diff < -1) {
        KFatal(diff == -2);
        diff = QueryBalance(TableEntryFromOffset(TableEntry->_LeftOffset));
        if (diff == 1) {
            RotateLeft(TableEntryFromOffset(TableEntry->_LeftOffset));
        }
        RotateRight(TableEntry);
    } else if (diff > 1) {
        KFatal(diff == 2);
        diff = QueryBalance(TableEntryFromOffset(TableEntry->_RightOffset));
        if (diff == -1) {
            RotateRight(TableEntryFromOffset(TableEntry->_RightOffset));
        }
        RotateLeft(TableEntry);
    }
}

LONG
KNodeTableBase::QueryBalance(
    __in_opt KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine returns the difference between the right and left subtree heights.

Arguments:

    TableEntry  - Supplies the table entry.

Return Value:

    FALSE   - The table entry is not balanced.

    TRUE    - The table entry is balanced.

--*/

{
    LONG leftHeight;
    LONG rightHeight;

    if (!TableEntry) {
        return 0;
    }

    //
    // Figure out the height of the left and right sub-trees and check the difference.
    //

    leftHeight = TableEntry->_LeftOffset ? TableEntryFromOffset(TableEntry->_LeftOffset)->_SubtreeHeight : 0;
    rightHeight = TableEntry->_RightOffset ? TableEntryFromOffset(TableEntry->_RightOffset)->_SubtreeHeight : 0;

    return (rightHeight - leftHeight);
}

VOID
KNodeTableBase::RotateRight(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine rotates the given table entry to the right in the tree.

Arguments:

    TableEntry  - Supplies the table entry to rotate to the right.

Return Value:

    None.

--*/

{
    KTableEntry* parent = TableEntryFromOffset(TableEntry->_ParentOffset);
    KTableEntry* pivot = TableEntryFromOffset(TableEntry->_LeftOffset);

    TableEntry->_LeftOffset = pivot->_RightOffset;
    if (TableEntry->_LeftOffset) {
        TableEntryFromOffset(TableEntry->_LeftOffset)->_ParentOffset = OffsetFromTableEntry(TableEntry);
    }

    pivot->_RightOffset = OffsetFromTableEntry(TableEntry);
    TableEntryFromOffset(pivot->_RightOffset)->_ParentOffset = OffsetFromTableEntry(pivot);

    if (!parent) {
        _Root = pivot;
    } else if (TableEntryFromOffset(parent->_LeftOffset) == TableEntry) {
        parent->_LeftOffset = OffsetFromTableEntry(pivot);
    } else {
        parent->_RightOffset = OffsetFromTableEntry(pivot);
    }
    pivot->_ParentOffset = OffsetFromTableEntry(parent);

    SetHeight(TableEntry);
    SetHeight(pivot);
}

VOID
KNodeTableBase::RotateLeft(
    __inout KTableEntry* TableEntry
    )

/*++

Routine Description:

    This routine rotates the given table entry to the left in the tree.

Arguments:

    TableEntry  - Supplies the table entry to rotate to the left.

Return Value:

    None.

--*/

{
    KTableEntry* parent = TableEntryFromOffset(TableEntry->_ParentOffset);
    KTableEntry* pivot = TableEntryFromOffset(TableEntry->_RightOffset);

    TableEntry->_RightOffset = pivot->_LeftOffset;
    if (TableEntry->_RightOffset) {
        TableEntryFromOffset(TableEntry->_RightOffset)->_ParentOffset = OffsetFromTableEntry(TableEntry);
    }

    pivot->_LeftOffset = OffsetFromTableEntry(TableEntry);
    TableEntryFromOffset(pivot->_LeftOffset)->_ParentOffset = OffsetFromTableEntry(pivot);

    if (!parent) {
        _Root = pivot;
    } else if (TableEntryFromOffset(parent->_LeftOffset) == TableEntry) {
        parent->_LeftOffset = OffsetFromTableEntry(pivot);
    } else {
        parent->_RightOffset = OffsetFromTableEntry(pivot);
    }
    pivot->_ParentOffset = OffsetFromTableEntry(parent);

    SetHeight(TableEntry);
    SetHeight(pivot);
}

BOOLEAN
KNodeTableBase::VerifyTable(
    )

/*++

Routine Description:

    This routine verifies the integrity of the table:  order and balance.

Arguments:

    None.

Return Value:

    FALSE   - There is an integrity error in the table.

    TRUE    - The table passed the integrity check.

--*/

{
    KTableEntry* p = NULL;
    KTableEntry* n;
    LONG diff;
    ULONG currentHeight;

    for (n = First(); n; n = Next(n)) {

        if (p) {

            //
            // The element that comes before this one should be strictly less than this one.
            //

            diff = Compare(p, n);
            if (diff >= 0) {
                KAssert(FALSE);
                return FALSE;
            }
        }

        //
        // All nodes should obey the AVL balance rule:  Heights are off by no more than 1.
        //

        diff = QueryBalance(n);

        if (diff < -1 || diff > 1) {
            KAssert(FALSE);
            return FALSE;
        }

        //
        // This nodes recorded sub-tree height should be exactly equal to the max of its two sub-tree heights plus 1.
        //

        currentHeight = n->_SubtreeHeight;
        SetHeight(n);

        if (currentHeight != n->_SubtreeHeight) {
            KAssert(FALSE);
            return FALSE;
        }

        //
        // All link back pointers should be consistent.
        //

        if (n->_ParentOffset) {
            if (TableEntryFromOffset(TableEntryFromOffset(n->_ParentOffset)->_LeftOffset) != n &&
                    TableEntryFromOffset(TableEntryFromOffset(n->_ParentOffset)->_RightOffset) != n) {

                KAssert(FALSE);
                return FALSE;
            }
        }

        if (n->_RightOffset) {
            if (TableEntryFromOffset(TableEntryFromOffset(n->_RightOffset)->_ParentOffset) != n) {
                KAssert(FALSE);
                return FALSE;
            }
        }

        if (n->_LeftOffset) {
            if (TableEntryFromOffset(TableEntryFromOffset(n->_LeftOffset)->_ParentOffset) != n) {
                KAssert(FALSE);
                return FALSE;
            }
        }

        //
        // Remember this node as the previous node for checking the order.
        //

        p = n;
    }

    return TRUE;
}
