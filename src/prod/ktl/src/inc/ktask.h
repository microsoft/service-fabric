


/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KTask

    Description:
      Kernel Tempate Library (KTL)

      Describes a general-purpose task interface for instrumentation.

    History:
      raymcc          03-Apr-2012         Initial version.

--*/

#pragma once

class KITaskManager;

// KITask
//
// Implemented by task providers.
// Models the execution of a single task instance.
//
//


class KITask  : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KITask);
public:
    // ExecuteAsync
    //
    // Models the execution of a single task instance.
    //
    virtual NTSTATUS
    ExecAsync(
        __in     KIDomNode::SPtr& InParameters,
        __out    KIDomNode::SPtr& Result,
        __out    KIDomNode::SPtr& ErrorDetail,
        __in_opt KAsyncContextBase::CompletionCallback Callback,
        __in_opt KAsyncContextBase* const ParentContext = nullptr
        ) = 0;
};

inline KITask::KITask() {}
inline KITask::~KITask() {}

class KITaskFactory : public KObject<KITaskFactory>, public KShared<KITaskFactory>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KITaskFactory);

public:

    // NewTask
    //
    // This is called by the task system to create a new task instance. The
    // task does not begin execution until KITask::ExecAsync is called.
    //
    virtual NTSTATUS
    NewTask(
        __in   KUri&         TaskTypeId,
        __in   KSharedPtr<KITaskManager> Manager,
        __out  KITask::SPtr& NewTask
        ) = 0;;
};


inline KITaskFactory::KITaskFactory() {}
inline KITaskFactory::~KITaskFactory() {}

//
//  KITable
//
//  Models generic table access.
//
//  This is synchronous and assumes fast in-memory access. Concurrency
//  control is limited to ensuring coherence in the table, but there is currently no
//  transaction support across multiple users.
//
//
//
class KITable : public KObject<KITable>, public KShared<KITable>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KITable);
public:
    class KIResultSet;

    enum { eSingleton = 1, eMultiRow = 2 };

    struct RowMetadata
    {
        ULONGLONG _Timestamp;
        ULONGLONG _Version;

        RowMetadata()
        {
            _Timestamp = _Version = 0;
        }
    };


    // Query
    //
    // Executes a query over the table.
    //
    // Parameters:
    //      QueryText       The text of the query.  For singleton tables the query string is always L"GET"
    //
    //      ResultSet       Receives the result set iterator. There will always just be one object in singleton tables.
    //
    // Return value:
    //    STATUS_SUCCESS    If the query is legal, even though it may return no rows.
    //    INVALID_PARAMETER If the query is invalid.
    //
    virtual NTSTATUS
    Query(
        __in  KStringView& QueryText,
        __out KSharedPtr<KIResultSet>& ResultSet
        ) = 0;


    // Get
    //
    // Gets a row based on its key.  For singleton tables, the Key can be a null/empty KStringView.
    //
    // Parameters:
    //      Key             The key value if the table is a multi-row table, or an empty KStringView for singletons.
    //      Row             Receives the object for the specified row. This is a COPY of what is stored in the database.
    //      Metadata        If present, receives the metadata for the row after the update.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //
    //
    virtual NTSTATUS
    Get(
        __in  const KStringView& Key,
        __out KIMutableDomNode::SPtr& Row,
        __out RowMetadata* Metadata = nullptr
        ) = 0;



    // Update
    //
    // Updates a row in the table.
    //
    // Parameters:
    //      Key                 The key to the row to be updated. This is an empty string for singleton tables.
    //      Row                 The row object which will replace the existing one for the key.  A COPY is made
    //                          and stored.
    //      ProposedRowVersion  On entry, if this is zero, the update will be performed. If nonzero, the update
    //                          is only performed if the resulting version will match the proposed version.
    //      Metadata            If present, receives the metadata for the row after the update.
    //
    //
    // Return value:
    //  OBJECT_NAME_NOT_FOUND   If the row cannot be located for update.
    //
    virtual NTSTATUS
    Update(
        __in    const KStringView& Key,
        __in    KIMutableDomNode::SPtr& Row,
        __in    ULONGLONG ProposedRowVersion,
        __out   RowMetadata* Metadata = nullptr
        ) = 0;

    // Insert
    //
    // Adds a new row to the table under the specified key.  If the key already exists, this will fail, meaning
    // an update is required.
    //
    // The version number for the row will always be 1 after completion.
    //
    // Parameters:
    //      Key         The key value to the row to be updated.  This is an empty string for singleton tables.
    //      Row         The object to be inserted into the table. A COPY of this node is made and stored.
    //      Metadata    If present, receives the metadata for the row after the update.
    //
    // Return value:
    //      OBJECT_NAME_EXISTS if a key collision occurs where the row already exists.
    //
    virtual NTSTATUS
    Insert(
        __in    const KStringView& Key,
        __in    KIMutableDomNode::SPtr& Row,
        __out   RowMetadata* Metadata = nullptr
        ) = 0;

    // Delete
    //
    // Removes the row associated with the specified key.
    //
    // Parameters:
    //      Key         The key for which the row should be deleted.  This is an empty string for singleton tables.
    //
    virtual NTSTATUS
    Delete(
        __in    KStringView& Key
        ) = 0;


    // Truncate
    //
    // Removes all the rows from the table.
    //
    virtual NTSTATUS
    Truncate() = 0;


    // KIResultSet
    //
    // An iterator for use with multi-row tables
    //
    class KIResultSet : public KShared<KIResultSet>, public KObject<KIResultSet>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(KIResultSet);
    public:
        // Next
        //
        // Retrieves the next table row in the query. Note that this returns
        // a copy of what is in the database, so it is safe to modify the returned value.
        //
        //
        // TBD: Coherence rules on what occurs if you are enumerating a table while
        // simultaneously modifying it.
        //
        // Parameters:
        //  Row         Receives the next row in the result set. This is a COPY of what
        //              is stored in the database.
        //
        // Return value:
        //   STATUS_SUCCESS if an object was returned.
        //   STATUS_NO_MORE_ENTRIES if the result set is exhausted.
        //
        virtual NTSTATUS
        Next(
            __out   KString::SPtr& Key,
            __out   KIMutableDomNode::SPtr& Row,
            __out   RowMetadata* Metadata = nullptr
            ) = 0;
     };
};

inline KITable::KITable() {}
inline KITable::~KITable() {}

//
//  KIDb
//
//  Models a generic database
//
class KIDb : public KShared<KIDb>, public KObject<KIDb>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KIDb);

public:

    // CreateTable
    //
    // Creates a table.  The Uri should be relative.
    //
    // Parameters:
    //      TableId         A relative URI.
    //      Flags           Either eSingleton | eMultiRow
    //      Table           Receives the table
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_EXISTS if the table already exists.
    //
    virtual NTSTATUS
    CreateTable(
        __in  KUri& TableId,
        __in  ULONG Flags,
        __out KITable::SPtr& Table
        )
        = 0;

    // DeleteTable
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND if the table name does not exist.
    //
    virtual NTSTATUS
    DeleteTable(
        __in KUri& TableId
        ) = 0;

    // GetTable
    //
    // Opens a table for access.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND if the table name does not exist.
    //
    virtual NTSTATUS
    GetTable(
        __in  KUri& TableId,
        __out KITable::SPtr& Table
        ) = 0;
};

inline KIDb::KIDb() {}
inline KIDb::~KIDb() {}

//
//  KITaskManager
//
//  Proposed interface to task engine.
//
//
class KITaskManager : public KShared<KITaskManager>, public KObject<KITaskManager>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KITaskManager);

public:

    enum { eSerialExecution = 1, eParallelExecution = 2 };


    // RegisterTaskFactory
    //
    // Used to register a task implementation.
    //
    // Parameters:
    //      TaskTypeId      Uri of the task type.
    //      Flags           If eSerialExecution, a second execution will be delayed until
    //                      any pending execution is completed. If eParallelExecution,
    //                      execution of new requests proceeds immediately.
    //      TaskFactory     The factory interface for the task type.
    //
    virtual NTSTATUS
    RegisterTaskFactory(
        __in   KUri& TaskTypeId,
        __in   ULONG Flags,
        __in   KITaskFactory::SPtr& TaskFactory
        ) = 0;

    // Creates a task which can be executed.
    //
    virtual NTSTATUS
    NewTask(
        __in  KUri& TaskTypeId,
        __out KITask::SPtr& NewInstance
        ) = 0;


    // GetTaskDb
    //
    // Returns the in-memory task database.
    //
    virtual NTSTATUS
    GetTaskDb(
        __out KIDb::SPtr& Database
        ) = 0;

};


inline KITaskManager::KITaskManager() {}
inline KITaskManager::~KITaskManager() {}



class KInMemoryDb : public KIDb
{
    friend class KLocalTaskManager;

public:
    virtual NTSTATUS
    CreateTable(
        __in  KUri& TableId,
        __in  ULONG Flags,
        __out KITable::SPtr& Table
        );

    virtual NTSTATUS
    DeleteTable(
        __in KUri& TableId
        );

    virtual NTSTATUS
    GetTable(
        __in KUri& TableId,
        __in KITable::SPtr& Table
        );

private:

    KInMemoryDb(
        __in KAllocator& Alloc
        )
        : _Tables(Alloc)
    {
    }

    struct Row : public KShared<Row>, public KObject<Row>
    {
        typedef KSharedPtr<Row> SPtr;

        KDynString             _Key;
        KITable::RowMetadata   _Metadata;
        KIMutableDomNode::SPtr _RowData;

        Row(
            __in KAllocator& Alloc
            )
            : _Key(Alloc)
            {
            }
    };

    struct Table : public KITable
    {
        typedef KSharedPtr<Table> SPtr;

        KSpinLock         _RowLock;
        KUri::SPtr        _TableId;
        ULONG             _TableFlags;
        KArray<Row::SPtr> _Rows;

        Table(
            __in KAllocator& Alloc
            )
            : _Rows(Alloc)
        {
        }

        virtual NTSTATUS
        Query(
            __in  KStringView& QueryText,
            __out KSharedPtr<KIResultSet>& ResultSet
            )
            {
                UNREFERENCED_PARAMETER(QueryText);
                UNREFERENCED_PARAMETER(ResultSet);
                return STATUS_NOT_IMPLEMENTED;
            }

        virtual NTSTATUS
        Get(
            __in  const KStringView& Key,
            __out KIMutableDomNode::SPtr& Row,
            __out RowMetadata* Metadata = nullptr
            );

        virtual NTSTATUS
        Update(
            __in    const KStringView& Key,
            __in    KIMutableDomNode::SPtr& Row,
            __in    ULONGLONG ProposedRowVersion,
            __out   RowMetadata* Metadata = nullptr
            );

        virtual NTSTATUS
        Insert(
            __in    const KStringView& Key,
            __in    KIMutableDomNode::SPtr& Row,
            __out   RowMetadata* Metadata = nullptr
            );

        virtual NTSTATUS
        Delete(
            __in    KStringView& Key
            );

        virtual NTSTATUS
        Truncate();
    };

    KSpinLock           _TableLock;
    KArray<Table::SPtr> _Tables;
};

//
//  KLocalTaskManager
//
//  Basic implementation
//
class KLocalTaskManager : public KITaskManager
{

public:
    static NTSTATUS
    Create(
        __in  KAllocator& Alloc,
        __out KITaskManager::SPtr& TaskMgr
        );

    virtual NTSTATUS
    RegisterTaskFactory(
        __in   KUri& TaskTypeId,
        __in   ULONG TaskTypeFlags,
        __in   KITaskFactory::SPtr& TaskFactory
        );

    // Creates a task which can be executed.
    //
    virtual NTSTATUS
    NewTask(
        __in  KUri& TaskTypeId,
        __out KITask::SPtr& NewInstance
        );


    // GetTaskDb
    //
    // Returns the in-memory task database.
    //
    virtual NTSTATUS
    GetTaskDb(
        __out KIDb::SPtr& Database
        );

private:
    struct TaskReg : public KObject<TaskReg>, public KShared<TaskReg>
    {
        typedef KSharedPtr<TaskReg> SPtr;

        KUri::SPtr          _TaskUri;
        KITaskFactory::SPtr _Factory;
    };

    KLocalTaskManager(
        __in KAllocator& Alloc
        )
        : _TaskRegs(Alloc), _Allocator(Alloc)
    {
    }

    BOOLEAN
    Find(
        __in KUri& TaskUri,
        __out KITaskFactory::SPtr& _Factory
        );

    KArray<TaskReg::SPtr>  _TaskRegs;
    KAllocator&            _Allocator;
    KSpinLock              _DbLock;
    KIDb::SPtr             _Database;
};
