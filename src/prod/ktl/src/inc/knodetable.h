/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    knodetable.h

Abstract:

    This file defines an node table class.  This class implements an AVL tree without allocating any memory.  This AVL tree
    implementation does not allow any duplicates.

Author:

    Norbert P. Kusters (norbertk) 29-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

class KNodeTableBase;

class KTableEntry {

    K_DENY_COPY(KTableEntry);

    public:

        ~KTableEntry(
            );

        NOFAIL
        KTableEntry(
            );

        BOOLEAN
        IsInserted(
            );

    private:

        friend class KNodeTableBase;

        ULONGLONG _ParentOffset;
        ULONGLONG _LeftOffset;
        ULONGLONG _RightOffset;
        ULONG _SubtreeHeight;
        ULONG _Reserved;

};

C_ASSERT(sizeof(KTableEntry) == 4*sizeof(ULONGLONG));

class KNodeTableBase {

    public:

        typedef KDelegate<LONG (__in const VOID* FirstRecord, __in const VOID* SecondRecord)> BaseCompareFunction;

        ~KNodeTableBase(
            );

        NOFAIL
        KNodeTableBase(
            __in ULONG LinksOffset,
            __in BaseCompareFunction Compare,
            __in_opt VOID* BaseAddress = NULL,
            __in_opt ULONGLONG RootOffset = 0,
            __in_opt ULONG Count = 0
            );

        VOID
        SetRoot(
            __in VOID* BaseAddress,
            __in ULONGLONG RootOffset,
            __in ULONG Count
            );

        KTableEntry*
        Lookup(
            __in const VOID* SearchRecord
            );

        KTableEntry*
        LookupEqualOrNext(
            __in const VOID* SearchRecord
            );

        KTableEntry*
        LookupEqualOrPrevious(
            __in const VOID* SearchRecord
            );

        BOOLEAN
        Insert(
            __inout KTableEntry* TableEntry
            );

        VOID
        Remove(
            __inout KTableEntry* TableEntry
            );

        KTableEntry*
        First(
            );

        KTableEntry*
        Last(
            );

        KTableEntry*
        Next(
            __in KTableEntry* TableEntry
            );

        KTableEntry*
        Previous(
            __in KTableEntry* TableEntry
            );

        ULONG
        Count(
            );

        ULONGLONG
        QueryRootOffset(
            );

        BOOLEAN
        VerifyTable(
            );

    private:

        K_DENY_COPY(KNodeTableBase);

        KTableEntry*
        TableEntryFromOffset(
            __in ULONGLONG Offset
            );

        ULONGLONG
        OffsetFromTableEntry(
            __in KTableEntry* TableEntry
            );

        LONG
        Compare(
            __in const VOID* SearchRecord,
            __in KTableEntry* TableEntry
            );

        LONG
        Compare(
            __in KTableEntry* First,
            __in KTableEntry* Second
            );

        VOID
        BalanceUp(
            __inout_opt KTableEntry* TableEntry
            );

        VOID
        SetHeight(
            __inout KTableEntry* TableEntry
            );

        VOID
        Rebalance(
            __inout KTableEntry* TableEntry
            );

        LONG
        QueryBalance(
            __in_opt KTableEntry* TableEntry
            );

        VOID
        RotateRight(
            __inout KTableEntry* TableEntry
            );

        VOID
        RotateLeft(
            __inout KTableEntry* TableEntry
            );

        VOID* _BaseAddress;
        KTableEntry* _Root;
        ULONG _Count;
        ULONG _LinksOffset;
        BaseCompareFunction _Compare;

};

template <class T, class KT = T>
class KNodeTable {

    K_DENY_COPY(KNodeTable);

    public:

        typedef KDelegate<LONG (__in KT& First, __in KT& Second)> CompareFunction;

        NOFAIL
        KNodeTable(
            __in ULONG LinksOffset,
            __in CompareFunction Compare,
            __in_opt VOID* BaseAddress = NULL,
            __in_opt ULONGLONG RootOffset = 0,
            __in_opt ULONG Count = 0
            ) :
            _CompareProxy(Compare),
            _Compare(&_CompareProxy, &BaseCompareProxy::Compare),
            _BaseTable(LinksOffset, _Compare, BaseAddress, RootOffset, Count)
        {
            _KeyOffset = (ULONG) (((ULONG_PTR) static_cast<KT*>(reinterpret_cast<T*>(1))) - 1);
            _LinksOffset = LinksOffset;
        }

        VOID
        SetRoot(
            __in VOID* BaseAddress,
            __in ULONGLONG RootOffset,
            __in ULONG Count
            )
        {
            _BaseTable.SetRoot(BaseAddress, RootOffset, Count);
        }

        T*
        Lookup(
            __in KT& SearchKey
            )
        {
            return RecordFromTableEntry(_BaseTable.Lookup((VOID*) &SearchKey));
        }

        T*
        LookupEqualOrNext(
            __in KT& SearchKey
            )
        {
            return RecordFromTableEntry(_BaseTable.LookupEqualOrNext((VOID*) &SearchKey));
        }

        T*
        LookupEqualOrPrevious(
            __in KT& SearchKey
            )
        {
            return RecordFromTableEntry(_BaseTable.LookupEqualOrPrevious((VOID*) &SearchKey));
        }

        BOOLEAN
        Insert(
            __inout T& Record
            )
        {
            return _BaseTable.Insert(TableEntryFromRecord(Record));
        }

        VOID
        Remove(
            __inout T& Record
            )
        {
            _BaseTable.Remove(TableEntryFromRecord(Record));
        }

        T*
        First(
            )
        {
            return RecordFromTableEntry(_BaseTable.First());
        }

        T*
        Last(
            )
        {
            return RecordFromTableEntry(_BaseTable.Last());
        }

        T*
        Next(
            __in T& Record
            )
        {
            return RecordFromTableEntry(_BaseTable.Next(TableEntryFromRecord(Record)));
        }

        T*
        Previous(
            __in T& Record
            )
        {
            return RecordFromTableEntry(_BaseTable.Previous(TableEntryFromRecord(Record)));
        }

        ULONG
        Count(
            )
        {
            return _BaseTable.Count();
        }

        ULONGLONG
        QueryRootOffset(
            )
        {
            return _BaseTable.QueryRootOffset();
        }

        BOOLEAN
        VerifyTable(
            )
        {
            return _BaseTable.VerifyTable();
        }

    private:

        class BaseCompareProxy {

            public:

                BaseCompareProxy(
                    __in CompareFunction Compare
                    )
                {
                    _Compare = Compare;
                }

                LONG
                Compare(
                    __in const VOID* First,
                    __in const VOID* Second
                    )
                {
                    return _Compare(*((KT*) First), *((KT*) Second));
                }

            private:

               CompareFunction _Compare;

        };

        KTableEntry*
        TableEntryFromRecord(
            __in const T& Record
            )
        {
            return (KTableEntry*) (((UCHAR*) &Record) + _KeyOffset + _LinksOffset);
        }

        T*
        RecordFromTableEntry(
            __in const KTableEntry* TableEntry
            )
        {
            if (!TableEntry) {
                return NULL;
            }
            return (T*) ((UCHAR*) TableEntry - _LinksOffset - _KeyOffset);
        }

        ULONG _KeyOffset;
        ULONG _LinksOffset;
        BaseCompareProxy _CompareProxy;
        KNodeTableBase::BaseCompareFunction _Compare;
        KNodeTableBase _BaseTable;

};
