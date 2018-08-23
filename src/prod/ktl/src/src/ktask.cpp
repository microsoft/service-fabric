/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KTask

    Description:
      Kernel Tempate Library (KTL)

      Task manager

    History:
      raymcc          20-Feb-2012         Initial version.

--*/

#pragma once

#include <ktl.h>

//
//  KLocalTaskManager::Create
//
NTSTATUS
KLocalTaskManager::Create(
    __in KAllocator& Alloc,
    __out KITaskManager::SPtr& TaskMgr
    )
{
    TaskMgr = _new(KTL_TAG_TASK, Alloc) KLocalTaskManager(Alloc);
    if (!TaskMgr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

//
//  KLocalTaskManager::RegisterTaskFactory
//
NTSTATUS
KLocalTaskManager::RegisterTaskFactory(
    __in   KUri& TaskTypeId,
    __in   ULONG Flags,
    __in   KITaskFactory::SPtr& TaskFactory
    )
{
    NTSTATUS Res;
    KITaskFactory::SPtr Existing;

    UNREFERENCED_PARAMETER(Flags);

    if (!TaskTypeId.IsValid() || !TaskFactory)
    {
        return STATUS_INVALID_PARAMETER;
    }

    BOOLEAN BRes = Find(TaskTypeId, Existing);
    if (BRes)
    {
        return STATUS_OBJECT_NAME_EXISTS;
    }

    TaskReg::SPtr NewReg = _new(KTL_TAG_TASK, _Allocator) TaskReg;

    if (!NewReg)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewReg->_Factory = TaskFactory;
    Res = TaskTypeId.Clone(NewReg->_TaskUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = _TaskRegs.Append(NewReg);

    if (!NT_SUCCESS(Res))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

//
//  KLocalTaskManager::NewTask
//
NTSTATUS
KLocalTaskManager::NewTask(
    __in  KUri& TaskTypeId,
    __out KITask::SPtr& NewInstance
    )
{
    KITaskFactory::SPtr Factory;
    BOOLEAN BRes = Find(TaskTypeId, Factory);
    if (!BRes)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    NTSTATUS Res = Factory->NewTask(TaskTypeId, this, NewInstance);
    return Res;
}

//
//  KLocalTaskManager::GetTaskDb
//
NTSTATUS
KLocalTaskManager::GetTaskDb(
    __out KIDb::SPtr& Database
    )
{
    _DbLock.Acquire();
    KFinally([&](){ _DbLock.Release(); });

    if (_Database)
    {
        Database = _Database;
        return STATUS_SUCCESS;
    }

    // If here, there is no database yet.  Create one.
    //
    KInMemoryDb* NewDb = _new(KTL_TAG_TASK, GetThisAllocator()) KInMemoryDb(GetThisAllocator());
    if (!NewDb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _Database = NewDb;
    Database = _Database;
    return STATUS_SUCCESS;
}



//
// KLocalTaskManager::Find
//
BOOLEAN
KLocalTaskManager::Find(
    __in  KUri& TaskUri,
    __out KITaskFactory::SPtr& _Factory
    )
{
    if (!TaskUri.IsValid())
    {
        return FALSE;
    }

    for (ULONG i = 0; i < _TaskRegs.Count(); i++)
    {
        TaskReg::SPtr Current = _TaskRegs[i];

        if (Current->_TaskUri->Compare(TaskUri) == TRUE)
        {
            _Factory = Current->_Factory;
            return TRUE;
        }
    }

    return FALSE;
}


//
//  KInMemoryDb::CreateTable
//
NTSTATUS
KInMemoryDb::CreateTable(
    __in  KUri& TableId,
    __in  ULONG Flags,
    __out KITable::SPtr& Table
    )
{
    NTSTATUS Res;
    KITable::SPtr Existing;

    if (GetTable(TableId, Existing) == STATUS_SUCCESS)
    {
        return STATUS_OBJECT_NAME_EXISTS;
    }

    // If here, we can create the new table.
    //
    KInMemoryDb::Table::SPtr NewTable = _new(KTL_TAG_TASK, GetThisAllocator()) KInMemoryDb::Table(GetThisAllocator());
    if (!NewTable)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Res = TableId.Clone(NewTable->_TableId);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    NewTable->_TableFlags = Flags;

    _TableLock.Acquire();
    Res = _Tables.Append(NewTable);
    _TableLock.Release();

    if (NT_SUCCESS(Res))
    {
        Table = (KITable::SPtr&) NewTable;
    }
    return STATUS_SUCCESS;
}

//
//  KInMemoryDb::DeleteTable
//
NTSTATUS
KInMemoryDb::DeleteTable(
    __in KUri& TableId
    )
{
    _TableLock.Acquire();
    KFinally([&](){ _TableLock.Release(); });

    for (ULONG i = 0; i < _Tables.Count(); i++)
    {
        Table::SPtr Current = _Tables[i];
        if (Current->_TableId->Compare(TableId))
        {
            _Tables.Remove(i);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}

//
// KInMemoryDb::GetTable
//
NTSTATUS
KInMemoryDb::GetTable(
    __in KUri& TableId,
    __in KITable::SPtr& Table
    )
{
    _TableLock.Acquire();
    KFinally([&](){ _TableLock.Release(); });

    for (ULONG i = 0; i < _Tables.Count(); i++)
    {
        Table::SPtr Current = _Tables[i];
        if (Current->_TableId->Compare(TableId))
        {
            Table = (KITable::SPtr&) Current;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}



//
// KInMemoryDb::Table::Get
//
NTSTATUS
KInMemoryDb::Table::Get(
     __in  const KStringView& Key,
     __out KIMutableDomNode::SPtr& Row,
     __out RowMetadata* Metadata
     )
{
    NTSTATUS Res;
    _RowLock.Acquire();
    KFinally([&](){ _RowLock.Release(); });

    for (ULONG i = 0; i < _Rows.Count(); i++)
    {
        Row::SPtr Current = _Rows[i];

        if (Key.Compare(Current->_Key) == 0)
        {
            Res = KDom::CloneAs((KIDomNode::SPtr&)Current->_RowData, Row);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }
            if (Metadata)
            {
                *Metadata = Current->_Metadata;
            }
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}

//
//  KInMemoryDb::Table::Delete
//
NTSTATUS
KInMemoryDb::Table::Delete(
     __in  KStringView& Key
     )
{
    _RowLock.Acquire();
    KFinally([&](){ _RowLock.Release(); });

    for (ULONG i = 0; i < _Rows.Count(); i++)
    {
        Row::SPtr Current = _Rows[i];
        if (Key.Compare(Current->_Key) == 0)
        {
            _Rows.Remove(i);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}


//
//  KInMemoryDb::Table::Update
//
NTSTATUS
KInMemoryDb::Table::Update(
    __in    const KStringView& Key,
    __in    KIMutableDomNode::SPtr& Row,
    __in    ULONGLONG ProposedRowVersion,
    __out   RowMetadata* Metadata
    )
{
    NTSTATUS Res;

    _RowLock.Acquire();
    KFinally([&](){ _RowLock.Release(); });

    for (ULONG i = 0; i < _Rows.Count(); i++)
    {
        KInMemoryDb::Row::SPtr Current = _Rows[i];

        if (Key.Compare(Current->_Key) == 0)
        {

            if ( ProposedRowVersion &&
                 (Current->_Metadata._Version +1 != ProposedRowVersion)
               )
            {
                return STATUS_REVISION_MISMATCH;
            }
            KIMutableDomNode::SPtr Copy;
            Res = KDom::CloneAs((KIDomNode::SPtr&)Row, Copy);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }
            Current->_RowData = Copy;
            Current->_Metadata._Version++;
            Current->_Metadata._Timestamp = (ULONGLONG) KNt::GetSystemTime();

            if (Metadata)
            {
                *Metadata = Current->_Metadata;
            }
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}


//
//  KInMemoryDb::Table::Insert
//
NTSTATUS
KInMemoryDb::Table::Insert(
     __in  const KStringView& Key,
     __out KIMutableDomNode::SPtr& Row,
     __out RowMetadata* Metadata
     )
{
    UNREFERENCED_PARAMETER(Metadata);

    NTSTATUS Res;
    _RowLock.Acquire();
    KFinally([&](){ _RowLock.Release(); });

    if (_Rows.Count() > 0 && (_TableFlags & eSingleton))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < _Rows.Count(); i++)
    {
        Row::SPtr Current = _Rows[i];

        if (Key.Compare(Current->_Key) == 0)
        {
            return STATUS_OBJECT_NAME_EXISTS;
        }
    }

    // If here, append.
    //
    Row::SPtr NewRow = _new(KTL_TAG_TASK, GetThisAllocator()) KInMemoryDb::Row(GetThisAllocator());
    if (!NewRow)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    BOOLEAN bRes = NewRow->_Key.Resize(Key.Length() + 1);
    if (!bRes)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewRow->_Key.CopyFrom(Key);

    KIMutableDomNode::SPtr Copy;
    Res = KDom::CloneAs((KIDomNode::SPtr&)Row, Copy);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }
    NewRow->_RowData = Copy;
    NewRow->_Metadata._Version = 1;
    NewRow->_Metadata._Timestamp = (ULONGLONG) KNt::GetSystemTime();

    Res = _Rows.Append(NewRow);

    return Res;
}

//
//  KInMemoryDb::Table::Truncate
//
NTSTATUS
KInMemoryDb::Table::Truncate()
{
    _RowLock.Acquire();
    KFinally([&](){ _RowLock.Release(); });
   _Rows.Clear();
    return STATUS_SUCCESS;
}
