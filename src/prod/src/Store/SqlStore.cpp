// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

// Database table related constants
// !!! Change "WinFabricVote" to a generic name, like WinFabricStore, since this is a generic key-value store,
// or this should a parameter of SqlStore constructor? since FM may use SqlStore too
#define WIN_FABRIC_STORE_TABLE_NAME L"WinFabricVote"
#define WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L"[" WIN_FABRIC_STORE_TABLE_NAME L"]"
#define TYPE_COLUMN_SIZE 100
#define KEY_COLUMN_SIZE 100
#define VALUE_COLUMN_SIZE 8000

#define SQL_FAILED(rc) ((rc == SQL_ERROR) || (rc == SQL_INVALID_HANDLE))

static StringLiteral const ContextTrace = "Context";
static StringLiteral const ActionTrace = "Action";

namespace Store
{
    ILocalStoreSPtr SqlStoreFactory(std::wstring const & connectionString)
    {
        return static_pointer_cast<ILocalStore>(SqlStore::Create(connectionString));
    }
}

class SqlStore::SqlHandle
{
    DENY_COPY(SqlHandle);
public:
    SqlHandle(SQLSMALLINT type) : type_(type), rawHandle_(nullptr) {}

    virtual ~SqlHandle() = 0
    {
        if (rawHandle_)
        {
            FreeRawHandle(type_, rawHandle_);
        }
    }

    __declspec(property(get=getHandle)) SQLHANDLE Handle;
    SQLHANDLE getHandle() const { return rawHandle_; }

    static void TraceFailureStatic(SQLSMALLINT handleType, SQLHANDLE handle, StringLiteral traceType, wstring const & action, __out wstring * errorState = nullptr)
    {
        SQLWCHAR sqlState[6];
        SDWORD nativeError;
        SQLWCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
        SWORD messageSize;
        SQLRETURN rc= SQLGetDiagRec(handleType, handle, 1, sqlState, &nativeError, message, SQL_MAX_MESSAGE_LENGTH, &messageSize);

        if ( rc == SQL_SUCCESS) 
        {
            WriteWarning(
                traceType,
                "{0} failed with SQL ERROR \n\tSTATE={1}\n\tNative error={2}\n\t Message = {3}",
                action,
                static_cast<SQLWCHAR*>(sqlState),
                nativeError,
                static_cast<SQLWCHAR*>(message));

            if (errorState)
            {
                *errorState = wstring(sqlState);
            }
        }
        else
        {
            WriteWarning(traceType, "SQLGetDiagRec failed on {0} with {1}", handle, rc);
        }
    }

    void TraceFailure(StringLiteral traceType, wstring const & action, __out wstring * errorState = nullptr)
    {
        TraceFailureStatic(type_, rawHandle_, traceType, action, errorState);
    }

protected:
    ErrorCode Initialize(SQLHANDLE parent, SQLSMALLINT parentType = 0)
    {
        ASSERT_IFNOT(rawHandle_ == nullptr, "Already initialized");

        SQLRETURN returnCode = SQLAllocHandle(type_, parent, &rawHandle_);
        if (SQL_FAILED(returnCode))
        {
            if (parent != SQL_NULL_HANDLE)
            {
                TraceFailureStatic(parentType, parent, ContextTrace, L"SQLAllocHandle");
            }
            else
            {
                SqlStore::WriteWarning(ContextTrace, "SQLAllocHandle failed, type={0}", type_);
            }

            return ErrorCodeValue::StoreUnexpectedError;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode Initialize(shared_ptr<SqlHandle> const & parent)
    {
        ErrorCode errorCode = Initialize(parent->rawHandle_, parent->type_);
        if (errorCode.IsSuccess())
        {
            parent_ = parent;
        }

        return errorCode;
    }

    static void FreeRawHandle(SQLSMALLINT type, SQLHANDLE rawHandle)
    {
        SqlStore::WriteInfo(ContextTrace, "Free type {0} handle {1}", type, rawHandle);
        SQLRETURN sqlReturn = SQLFreeHandle(type, rawHandle);
        if (SQL_FAILED(sqlReturn))
        {
            TraceFailureStatic(type, rawHandle, ContextTrace, L"SQLFreeHandle");
        }
    }

    SQLSMALLINT type_;
    shared_ptr<SqlHandle> parent_;
    SQLHANDLE rawHandle_;
};

class SqlStore::SqlEnvironment : public SqlHandle
{
public:
    SqlEnvironment() : SqlHandle(SQL_HANDLE_ENV) {}

    ErrorCode Initialize()
    {
        return SqlHandle::Initialize(SQL_NULL_HANDLE);
    }

    ErrorCode SetAttribute(SQLINTEGER attribute, SQLPOINTER valuePtr, SQLINTEGER stringLength)
    {
        SQLRETURN returnCode = SQLSetEnvAttr(rawHandle_, attribute, valuePtr, stringLength);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Set environment attribute");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        return ErrorCodeValue::Success;
    }
};

class SqlStore::SqlConnection : public SqlHandle
{
public:
    SqlConnection(SqlStore & store)
        : storeWPtr_(store.shared_from_this()), SqlHandle(SQL_HANDLE_DBC), connected_(false)
    {
    }

    static void DisconnectSqlConnection(SQLHDBC connection)
    {
        SqlStore::WriteInfo(ContextTrace, "Disconnect SQL connection {0}", connection);
        SQLRETURN sqlReturn = SQLDisconnect(connection);
        if (SQL_FAILED(sqlReturn))
        {
            TraceFailureStatic(SQL_HANDLE_DBC, connection, ContextTrace, L"SQLDisconnect");
        }

        FreeRawHandle(SQL_HANDLE_DBC, connection);
    }

    virtual ~SqlConnection()
    {
        if (!connected_)
        {
            // base class destructor will free the handle
            return;
        }

        if (rawHandle_ != nullptr)
        {
            SQLHDBC connection = rawHandle_;
            rawHandle_ = nullptr;
            shared_ptr<SqlHandle> parent = move(parent_);

            // Post SQLDisconnect to a different thread since it may take a long time
            auto store = storeWPtr_.lock();
            if (store && (store->State.Value == FabricComponentState::Opened))
            {
                SqlStore::WriteInfo(ContextTrace, "Schedule background cleanup of connection {0}", connection);
                Threadpool::Post(
                    [connection, parent]
                    {
                        DisconnectSqlConnection(connection);
                    });
            }
            else
            {
                // Disconnect synchronously during shutdown, since we don't have to worry about long disconnect
                // time during shutdown, plus, threadpool post may fail during process shutdown.
                DisconnectSqlConnection(connection);
            }
        }
    }

    ErrorCode Connect(shared_ptr<SqlEnvironment> const & environment, wstring const & connectionString)
    {
        ErrorCode errorCode = Initialize(environment);
        if (!errorCode.IsSuccess())
        {
            return errorCode;
        }

        SQLWCHAR outConnectionString[1024];
        SQLSMALLINT outConnectionStringLen;
        SqlStore::WriteInfo(ContextTrace, "Connecting to store {0}", connectionString);
        SQLRETURN returnCode = SQLDriverConnect(
            rawHandle_,
            nullptr, 
            (SQLWCHAR *) connectionString.c_str(),
            SQL_NTS,
            outConnectionString,
            sizeof(outConnectionString)/sizeof(SQLWCHAR),
            &outConnectionStringLen,
            SQL_DRIVER_NOPROMPT);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Connect");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        WriteInfo(ContextTrace, "Connection {0} opened", rawHandle_);
        connected_ = true;

        returnCode = SQLSetConnectAttr(rawHandle_, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER *) SQL_AUTOCOMMIT_OFF, 0);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Set AutoCommit OFF");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        returnCode = SQLSetConnectAttr(rawHandle_, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER *) SQL_TXN_REPEATABLE_READ, 0);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Set Isolation level");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        return ErrorCodeValue::Success;
    }

    bool IsConnectionActive()
    {
        LONG connectionStatus;
        SQLINTEGER outLength;
        SQLRETURN returnCode = SQLGetConnectAttr(rawHandle_, SQL_COPT_SS_CONNECTION_DEAD, &connectionStatus, SQL_IS_INTEGER, &outLength);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Get connection attribute");
            return false;
        }

        return connectionStatus == SQL_CD_FALSE;
    }

    ErrorCode Commit()
    {
        SQLRETURN returnCode = SQLEndTran(SQL_HANDLE_DBC, rawHandle_, SQL_COMMIT);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ActionTrace, L"Transaction commit");
            return ErrorCodeValue::SqlStoreCommitTransFailed;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode Rollback()
    {
        SQLRETURN returnCode = SQLEndTran(SQL_HANDLE_DBC, rawHandle_, SQL_ROLLBACK);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ActionTrace, L"Transaction rollback");
            return ErrorCodeValue::SqlStoreRollbackTransFailed;
        }

        return ErrorCodeValue::Success;
    }

private:
    const SqlStore::WPtr storeWPtr_;
    bool connected_;
};

class SqlStore::SqlStatement : public SqlHandle
{
public:
    SqlStatement() : SqlHandle(SQL_HANDLE_STMT) {}

    ErrorCode Initialize(shared_ptr<SqlConnection> const & connection, wchar_t const * statementText)
    {
        ErrorCode errorCode = SqlHandle::Initialize(connection);
        if (!errorCode.IsSuccess()) return errorCode;

        SQLRETURN returnCode = SQLPrepare(rawHandle_, const_cast<SQLWCHAR*>(statementText), SQL_NTS);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ContextTrace, L"Prepare " + wstring(statementText));
            return ErrorCodeValue::StoreUnexpectedError;
        }

        return ErrorCodeValue::Success;
    }

    SQLRETURN BindStringParam(SQLUSMALLINT paramNumber, SQLULEN columnSize, wstring const & value, SQLLEN & size, wchar_t const * action)
    {
        SQLRETURN sqlReturn = SQLBindParameter(
            rawHandle_,
            paramNumber,
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WVARCHAR,
            columnSize,
            0,
            (SQLPOINTER) value.c_str(),
            size,
            &size);

        if (SQL_FAILED(sqlReturn))
        {
            TraceFailure(ActionTrace, action);
        }

        return sqlReturn;
    }

    SQLRETURN BindBinaryParam(SQLUSMALLINT paramNumber, SQLULEN columnSize, void const *buffer, SQLLEN & bufferLen, wchar_t const * action)
    {
        SQLRETURN sqlReturn = SQLBindParameter(
            rawHandle_,
            paramNumber,
            SQL_PARAM_INPUT,
            SQL_C_BINARY,
            SQL_BINARY,
            columnSize,
            0,
            (SQLPOINTER) buffer,
            bufferLen,
            &bufferLen);

        if (SQL_FAILED(sqlReturn))
        {
            TraceFailure(ActionTrace, action);
        }

        return sqlReturn;
    }

    SQLRETURN BindBigIntParam(SQLUSMALLINT paramNumber, void const*value, wchar_t const * action)
    {
        SQLRETURN sqlReturn = SQLBindParameter(
            rawHandle_,
            paramNumber,
            SQL_PARAM_INPUT,
            SQL_C_SBIGINT,
            SQL_BIGINT,
            0,
            0,
            (SQLPOINTER)value,
            0,
            NULL);

        if (SQL_FAILED(sqlReturn))
        {
            TraceFailure(ActionTrace, action);
        }

        return sqlReturn;
    }


    SQLRETURN Execute(wchar_t const * action, __out wstring * errorState = nullptr)
    {
        SQLRETURN returnCode = SQLExecute(rawHandle_);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ActionTrace, action, errorState);
        }

        return returnCode;
    }

    void FreeStatementResult()
    {
        SQLRETURN returnCode = SQLFreeStmt(rawHandle_, SQL_CLOSE);
        if (SQL_FAILED(returnCode))
        {
            TraceFailure(ActionTrace, L"SQLFreeStmt");
        }
    }
};

class SqlStore::SqlStatementExecution
{
    DENY_COPY(SqlStatementExecution);

public:
    SqlStatementExecution(SqlStatement & statement) : statement_(statement) {}

    ~SqlStatementExecution()
    {
        statement_.FreeStatementResult();
    }

    SQLRETURN Execute(wchar_t const * actionTrace, wstring * errorState = nullptr)
    {
        return statement_.Execute(actionTrace, errorState);
    }

private:
    SqlStatement & statement_;
};

class SqlStore::SqlHandleSet
{
    DENY_COPY(SqlHandleSet);

public:
    SqlHandleSet(SqlStore & store) : Connection(make_shared<SqlConnection>(store))
    {
    }

    ErrorCode PrepareStatements()
    {
        auto errorCode = GetTimeStmt.Initialize(Connection, L"SELECT SYSUTCDATETIME()");
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = EnumerateTypeStmt.Initialize(
            Connection,
            L"SELECT [Key], [Version], [Data] FROM " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" WHERE [Type]=?");
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = EnumerateKeyStmt.Initialize(
            Connection,
            L"SELECT [Key], [Version], [Data] FROM " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" WHERE [Type]=? AND [Key]=?");
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = UpdateStmt.Initialize(
            Connection,
            L"UPDATE " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" SET [Data] = ?, [Version] = [Version] + 1 WHERE [Type] = ? AND [Key] = ? AND ([Version] = ? OR ? <= 0)");
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = InsertStmt.Initialize(
            Connection,
            L"INSERT INTO " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" ([Type], [Key], [Version], [Data]) VALUES (?, ?, ?, ?)");
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = DeleteStmt.Initialize(
            Connection,
            L"DELETE " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" WHERE [Type] = ? AND [Key] = ? AND ([Version] = ? OR ? <= 0)");
        if (!errorCode.IsSuccess()) return errorCode;

        return DeleteAllStmt.Initialize(Connection, L"DELETE " WIN_FABRIC_STORE_TABLE_NAME_DELIMITED L" WHERE [Type] = ?");
    }

    shared_ptr<SqlConnection> Connection;
    SqlStatement GetTimeStmt;
    SqlStatement EnumerateTypeStmt;
    SqlStatement EnumerateKeyStmt;
    SqlStatement UpdateStmt;
    SqlStatement InsertStmt;
    SqlStatement DeleteStmt;
    SqlStatement DeleteAllStmt;
};

class SqlStore::SqlContext : public ComponentRoot
{
    DENY_COPY(SqlContext);

public:
    SqlContext(SqlStore & store, wstring const & connectionString)
        : store_(store)
        , connectionString_(connectionString)
        , environment_(make_shared<SqlEnvironment>())
        , recovering_(false)
    {
    }

    ~SqlContext()
    {
    }

    ErrorCode Initialize()
    {
        ErrorCode errorCode = environment_->Initialize();
        if (!errorCode.IsSuccess())
        {
            return errorCode;
        }

        errorCode = environment_->SetAttribute(SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (!errorCode.IsSuccess()) return errorCode;

        auto handleSet = make_shared<SqlHandleSet>(store_);
        errorCode = handleSet->Connection->Connect(environment_, connectionString_);
        if (!errorCode.IsSuccess()) return errorCode;

        errorCode = handleSet->PrepareStatements();
        if (errorCode.IsSuccess()) handleSet_ = move(handleSet);

        return errorCode;
    }

    shared_ptr<SqlHandleSet> GetHandles()
    {
        shared_ptr<SqlHandleSet> handleSet;
        {
            AcquireReadLock grab(lock_);
            handleSet = handleSet_;
        }

        if (!handleSet)
        {
            SqlStore::WriteNoise(ContextTrace, "GetHandles: no connection found");
            ResetHandles();
            return handleSet;
        }

        if (handleSet->Connection->IsConnectionActive())
        {
            SqlStore::WriteNoise(ContextTrace, "GetHandles: found active connection");
            return handleSet;
        }

        SqlStore::WriteWarning(ContextTrace, "GetHandles: SQL connection has been lost");
        ResetHandles();
        return nullptr;
    }

    void ResetHandles()
    {
        shared_ptr<SqlHandleSet> handleSet;
        {
            AcquireWriteLock grab(lock_);

            if (recovering_) return;

            // move handleSet_ to a local variable instead of resetting to avoid calling ~SqlHandleSet under lock
            handleSet = move(handleSet_);
            recovering_ = true;
        }

        // Post to a different thread since connection setup may take a long time
        SqlStore::WriteInfo(ContextTrace, "Start background recovery ...");
        auto root = CreateComponentRoot();
        Threadpool::Post(
            [root, this] 
            {
                SqlStore::WriteInfo(ContextTrace, "Trying to reconnect ...");
                auto newHandleSet = make_shared<SqlHandleSet>(store_);
                if (!newHandleSet->Connection->Connect(environment_, connectionString_).IsSuccess()
                    || !newHandleSet->PrepareStatements().IsSuccess())
                {
                    newHandleSet = nullptr;
                }

                AcquireWriteLock grab(lock_);
                handleSet_ = move(newHandleSet);
                recovering_ = false;
            });
    }

private:
    RwLock lock_;

    SqlStore & store_;
    wstring connectionString_;

    shared_ptr<SqlEnvironment> environment_;
    shared_ptr<SqlHandleSet> handleSet_;

    bool recovering_;
};

// Automatically rolls back if this goes out of scope without calling Commit.
class SqlStore::SqlTransaction : public TransactionBase
{
    DENY_COPY(SqlTransaction);
public:
    SqlTransaction(RwLock & transactionLock, SqlContext & sqlContext)
        : grabTransactionLock_(transactionLock), sqlContext_(sqlContext), handleSet_(sqlContext_.GetHandles()), isActive_(handleSet_)
    {
    }

    virtual ~SqlTransaction()
    {
        if (isActive_.load())
        {
            handleSet_->Connection->Rollback();
        }
    }

    virtual void AcquireLock() {}

    virtual void ReleaseLock() {}

    virtual ErrorCode Commit(::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber, Common::TimeSpan const timeout = Common::TimeSpan::MaxValue)
    {
        UNREFERENCED_PARAMETER(timeout);
        // SqlStore does not implement commit sequence number, hence returning -1 as the default value
        commitSequenceNumber = -1;
        if (isActive_.exchange(false))
        {
            ErrorCode errorCode = handleSet_->Connection->Commit();
            if (!errorCode.IsSuccess())
            {
                sqlContext_.ResetHandles();
            }

            return errorCode; 
        }

        return ErrorCodeValue::SqlStoreTransactionAlreadyCommitted;
    }

    virtual void Rollback()
    {
        if (isActive_.exchange(false))
        {
            handleSet_->Connection->Rollback();
        }
    }

    __declspec(property(get=get_IsActive, put=set_IsActive)) bool IsActive;
    bool get_IsActive() const {return isActive_.load(); }
    void set_IsActive(bool value) { isActive_.store(value); }

    static SqlHandleSet* GetHandles(ILocalStore::TransactionSPtr const & trans)
    {
        if (trans)
        {
            SqlTransaction *sqlTrans = static_cast<SqlTransaction *>(trans.get());
            return sqlTrans->handleSet_.get();
        }

        return nullptr;
    }

private:
    AcquireWriteLock grabTransactionLock_;
    SqlContext & sqlContext_;
    shared_ptr<SqlHandleSet> handleSet_;
    atomic_bool isActive_;
};

class SqlStore::SqlEnumeration : public EnumerationBase
{
    DENY_COPY(SqlEnumeration);

public:
    SqlEnumeration(SqlStatement * statement, wstring const & currentType, wstring const & currentKey)
        : statement_(statement),
        moveNextCalled_(false),
        currentType_(currentType),
        currentKey_(currentKey),
        currentValue_(),
        currentOperationLSN_(0)
    {
    }

    virtual ~SqlEnumeration() 
    { 
        if (statement_)
        {
            SQLFreeStmt(statement_->Handle, SQL_CLOSE); 
        }
    }

    virtual ErrorCode MoveNext()
    {
        if (!statement_)
        {
            SqlStore::WriteWarning(ActionTrace, "SqlEnumeration::MoveNext: statement handle is null");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        moveNextCalled_ = true;

        SQLRETURN returnCode = SQLFetch(statement_->Handle);
        if (SQL_FAILED(returnCode))
        {
            statement_->TraceFailure(ActionTrace, L"Fetch for enumeration query");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        if (returnCode == SQL_NO_DATA)
        {
            returnCode = SQLFreeStmt(statement_->Handle, SQL_CLOSE);
            if (SQL_FAILED(returnCode))
            {
                statement_->TraceFailure(ActionTrace, L"Clearing up enumeration statement");
                return ErrorCodeValue::StoreUnexpectedError;
            }
            return ErrorCodeValue::EnumerationCompleted;
        }

        //[key], [version], [data]
        WCHAR key[100];
        SQLLEN size;
        returnCode = SQLGetData(statement_->Handle, 1, SQL_C_WCHAR, key, sizeof(key)/sizeof(WCHAR), &size);
        if (SQL_FAILED(returnCode))
        {
            statement_->TraceFailure(ActionTrace, L"GetData for key");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        currentKey_ = key;

        returnCode = SQLGetData(statement_->Handle, 2, SQL_C_SBIGINT, &currentOperationLSN_, 0, &size);
        if (SQL_FAILED(returnCode))
        {
            statement_->TraceFailure(ActionTrace,  L"GetData for version");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        currentValue_.resize(8000);
        returnCode = SQLGetData(statement_->Handle, 3, SQL_C_BINARY, currentValue_.data(), currentValue_.size(), &size);
        if (SQL_FAILED(returnCode))
        {
            statement_->TraceFailure(ActionTrace, L"GetData for value");
            return ErrorCodeValue::StoreUnexpectedError;
        }

        if (size == SQL_NULL_DATA)
        {
            size = 0;
        }

        currentValue_.resize(size);
        return ErrorCodeValue::Success;
    }

    virtual ErrorCode CurrentOperationLSN(__int64 & operationLSN)
    {
        ASSERT_IF(!moveNextCalled_, "MoveNext needs to be called on the enumerator before accessing it.");
        // Extract the version
        operationLSN = currentOperationLSN_;
        return ErrorCodeValue::Success;
    }

    virtual ErrorCode CurrentLastModifiedFILETIME(FILETIME & fileTime)
    {
        fileTime.dwHighDateTime = 0;
        fileTime.dwLowDateTime = 0;
        return ErrorCodeValue::Success;
    }

    // Clears current contents and resizes to the size of the value.
    virtual ErrorCode CurrentType(wstring & buffer)
    {
        ASSERT_IF(!moveNextCalled_, "MoveNext needs to be called on the enumerator before accessing it.");
        buffer = currentType_;
        return ErrorCodeValue::Success;
    }

    virtual ErrorCode CurrentKey(wstring & buffer)
    {
        ASSERT_IF(!moveNextCalled_, "MoveNext needs to be called on the enumerator before accessing it.");
        buffer = currentKey_;
        return ErrorCodeValue::Success;
    }

    virtual ErrorCode CurrentValue(vector<unsigned _int8> & buffer)
    {
        ASSERT_IF(!moveNextCalled_, "MoveNext needs to be called on the enumerator before accessing it.");
        buffer = move(currentValue_);
        return ErrorCodeValue::Success;
    }

    virtual ErrorCode CurrentValueSize(size_t & size)
    {
        ASSERT_IF(!moveNextCalled_, "MoveNext needs to be called on the enumerator before accessing it.");
        size = currentValue_.size();
        return ErrorCodeValue::Success;
    }


private:
    SqlStatement * statement_;
    wstring currentType_;
    wstring currentKey_;
    vector<unsigned _int8> currentValue_;
    __int64 currentOperationLSN_;
    bool moveNextCalled_;
};

SqlStore::SPtr SqlStore::Create(wstring const & connectionString)
{
    return make_shared<SqlStore>(connectionString);
}

SqlStore::SqlStore(wstring const & connectionString)
{
    sqlContext_ = make_shared<SqlContext>(*this, connectionString);
}

ErrorCode SqlStore::OnOpen()
{
    if (sqlContext_->Initialize().IsSuccess())
    {
        if (IsTableMissing())
        {
            WriteError(ActionTrace, "Table {0} is missing", WIN_FABRIC_STORE_TABLE_NAME);
            return ErrorCodeValue::SqlStoreTableNotFound;
        }
    }

    // Always report success since initialization is best effort and there will be background retry later
    return ErrorCodeValue::Success;
}

ErrorCode SqlStore::OnClose()
{
    return ErrorCodeValue::Success;
}

void SqlStore::OnAbort()
{
}

::FABRIC_TRANSACTION_ISOLATION_LEVEL SqlStore::GetDefaultIsolationLevel()
{
    return FABRIC_TRANSACTION_ISOLATION_LEVEL::FABRIC_TRANSACTION_ISOLATION_LEVEL_READ_UNCOMMITTED;
}

Common::ErrorCode SqlStore::CreateTransaction(
    __out TransactionSPtr & transactionSPtr)
{
    // SqlTransaction constructor acquires a write lock on transactionLock_ at its very beginning
    transactionSPtr = make_shared<SqlTransaction>(transactionLock_, *sqlContext_);
    return ErrorCodeValue::Success;
}

bool SqlStore::IsTableMissing()
{
    shared_ptr<SqlHandleSet> handleSet = sqlContext_->GetHandles();
    if (!handleSet)
    {
        WriteWarning(ActionTrace, "IsTableMissing: failed to get handleSet"); 
        return false;
    }

    wstring selectStatementText;
    StringWriter selectStatementWriter(selectStatementText);
    selectStatementWriter.Write("select TABLE_NAME from INFORMATION_SCHEMA.TABLES where TABLE_NAME = '{0}'", WIN_FABRIC_STORE_TABLE_NAME);

    SqlStatement selectStatement;
    if (!selectStatement.Initialize(handleSet->Connection, selectStatementText.c_str()).IsSuccess())
    {
        return false;
    }

    SqlStatementExecution sqlStatementExecution(selectStatement);
    SQLRETURN returnCode = sqlStatementExecution.Execute(L"check table existence");
    if (SQL_FAILED(returnCode))
    {
        return false;
    }

    returnCode = SQLFetch(selectStatement.Handle);
    if (returnCode == SQL_NO_DATA)
    {
        return true;
    }

    return false;
}

Common::ErrorCode SqlStore::CreateEnumerationByTypeAndKey(
    __in TransactionSPtr const & transaction,
    __in std::wstring const & type,
    __in std::wstring const & keyStart,
    __out EnumerationSPtr & enumerationSPtr)
{
    enumerationSPtr = CreateKeyEnumeration(transaction, type, keyStart);
    return ErrorCodeValue::Success;
}

Common::ErrorCode SqlStore::CreateEnumerationByOperationLSN(
    __in TransactionSPtr const & transaction,
    __in _int64 fromOperationNumber,
    __out EnumerationSPtr & enumerationSPtr)
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(fromOperationNumber);
    UNREFERENCED_PARAMETER(enumerationSPtr);

    return ErrorCodeValue::NotImplemented;
}

Common::ErrorCode SqlStore::GetLastChangeOperationLSN(
    __in TransactionSPtr const & transaction,
    __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(operationLSN);

    return ErrorCodeValue::NotImplemented;
}

SqlStore::EnumerationSPtr SqlStore::CreateTypeEnumeration(TransactionSPtr const & transaction, wstring const & type)
{
    SqlHandleSet* handleSet = SqlTransaction::GetHandles(transaction);
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Transaction is not active");
        return make_shared<SqlEnumeration>(static_cast<SqlStatement*>(nullptr), type, L"");
    }

    SQLLEN typeSize = type.size() * sizeof(WCHAR);
    SQLRETURN returnCode = handleSet->EnumerateTypeStmt.BindStringParam(1, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type parameter to enumerate statement");
    if (SQL_FAILED(returnCode))
    {
        return make_shared<SqlEnumeration>(&(handleSet->EnumerateTypeStmt), type, L"");
    }

    handleSet->EnumerateTypeStmt.Execute(L"Enumerate type");
    return make_shared<SqlEnumeration>(&(handleSet->EnumerateTypeStmt), type, L"");
}

SqlStore::EnumerationSPtr SqlStore::CreateKeyEnumeration(TransactionSPtr const & transaction, wstring const & type, wstring const & key)
{
    SqlHandleSet* handleSet = SqlTransaction::GetHandles(transaction);
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Transaction is not active");
        return make_shared<SqlEnumeration>(static_cast<SqlStatement*>(nullptr), type, key);
    }

    SQLLEN typeSize = type.size() * sizeof(WCHAR);
    SQLRETURN returnCode = handleSet->EnumerateKeyStmt.BindStringParam(1, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type parameter to enumerate statement");
    if (SQL_FAILED(returnCode))
    {
        return make_shared<SqlEnumeration>(&(handleSet->EnumerateKeyStmt), type, key);
    }

    SQLLEN keySize = key.size() * sizeof(WCHAR);
    returnCode = handleSet->EnumerateKeyStmt.BindStringParam(2, KEY_COLUMN_SIZE, key, keySize, L"Bind key parameter to enumerate statement");
    if (SQL_FAILED(returnCode))
    {
        return make_shared<SqlEnumeration>(&(handleSet->EnumerateKeyStmt), type, key);
    }

    handleSet->EnumerateKeyStmt.Execute(L"Enumerate key");
    return make_shared<SqlEnumeration>(&(handleSet->EnumerateKeyStmt), type, key);
}

ErrorCode SqlStore::Insert(
    TransactionSPtr const & transaction,
    wstring const & type,
    wstring const & key,
    __in void const * value,
    size_t valueSizeInBytes,
    __int64 sequenceNumber)
{
    SqlHandleSet* handleSet = SqlTransaction::GetHandles(transaction);
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Transaction is not active");
        return ErrorCodeValue::StoreUnexpectedError;
    }

    if (sequenceNumber == ILocalStore::OperationNumberUnspecified)
    {
        sequenceNumber = 0;
    }

    SQLLEN typeSize = sizeof(WCHAR) * type.size();
    SQLRETURN returnCode = handleSet->InsertStmt.BindStringParam(1, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type parameter to insert statement");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SQLLEN keySize = sizeof(WCHAR) * key.size();
    returnCode = handleSet->InsertStmt.BindStringParam(2, KEY_COLUMN_SIZE, key, keySize, L"Bind key parameter to insert statement");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    __int64 version = sequenceNumber;
    returnCode = handleSet->InsertStmt.BindBigIntParam(3, &version, L"Bind version parameter to insert statement");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SQLLEN valueSize = (SQLLEN) valueSizeInBytes;
    returnCode = handleSet->InsertStmt.BindBinaryParam(4, VALUE_COLUMN_SIZE, value, valueSize, L"Bind value parameter to insert statement");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    wstring errorState;
    SqlStatementExecution sqlStatementExecution(handleSet->InsertStmt);
    returnCode = sqlStatementExecution.Execute(L"Insert", &errorState);
    if (returnCode == SQL_ERROR)
    {
        if (errorState == L"23000")
        {
            return ErrorCodeValue::SqlStoreDuplicateInsert;
        }
        else
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }
    }

    SQLLEN rowCount;
    returnCode = SQLRowCount(handleSet->InsertStmt.Handle, &rowCount);
    if (SQL_FAILED(returnCode))
    {
        handleSet->InsertStmt.TraceFailure(ActionTrace, L"Get number of rows effected by insert");
        return ErrorCodeValue::StoreUnexpectedError;
    }

    if (rowCount != 1)
    {
        handleSet->InsertStmt.TraceFailure(ActionTrace, L"Insert count");
        return ErrorCodeValue::StoreUnexpectedError;
    }

    return ErrorCodeValue::Success;
}

ErrorCode SqlStore::Update(
    TransactionSPtr const & transaction,
    wstring const & type,
    wstring const & key,
    __int64 checkSequenceNumber,
    wstring const & /*newKey*/,
    __in_opt void const * newValue,
    size_t valueSizeInBytes,
    __int64 sequenceNumber)
{
    ASSERT_IFNOT(
        sequenceNumber == ILocalStore::OperationNumberUnspecified,
        "SqlStore only supports SequenceNumberAuto");

    SqlHandleSet* handleSet = SqlTransaction::GetHandles(transaction);
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Transaction is not active");
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SQLLEN valueSize = (SQLLEN) valueSizeInBytes;
    SQLRETURN returnCode = handleSet->UpdateStmt.BindBinaryParam(1, VALUE_COLUMN_SIZE, newValue, valueSize, L"Bind value to update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SQLLEN typeSize = sizeof(WCHAR) * type.size();
    returnCode = handleSet->UpdateStmt.BindStringParam(2, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type to update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SQLLEN keySize = sizeof(WCHAR) * key.size();
    returnCode = handleSet->UpdateStmt.BindStringParam(3, KEY_COLUMN_SIZE, key, keySize, L"Bind key to update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    returnCode = handleSet->UpdateStmt.BindBigIntParam(4, &checkSequenceNumber, L"Bind checkSequenceNumber 1 to update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    returnCode = handleSet->UpdateStmt.BindBigIntParam(5, &checkSequenceNumber, L"Bind checkSequenceNumber 2 to update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    SqlStatementExecution sqlStatementExecution(handleSet->UpdateStmt);
    returnCode = sqlStatementExecution.Execute(L"Update");
    if (SQL_FAILED(returnCode))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    return ErrorCodeValue::Success;
}

ErrorCode SqlStore::Delete(
    TransactionSPtr const & transaction,
    wstring const & type,
    wstring const & key,
    __int64 checkSequenceNumber)
{
    SqlHandleSet* handleSet = SqlTransaction::GetHandles(transaction);
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Transaction is not active");
        return ErrorCodeValue::StoreUnexpectedError;
    }

    if (key.empty())
    {
        SQLLEN typeSize = sizeof(WCHAR) * type.size();
        SQLRETURN returnCode = handleSet->DeleteAllStmt.BindStringParam(1, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type to delete all");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }

        SqlStatementExecution sqlStatementExecution(handleSet->DeleteAllStmt);
        returnCode = sqlStatementExecution.Execute(L"Delete all");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }
    }
    else
    {
        SQLLEN typeSize = sizeof(WCHAR) * type.size();
        SQLRETURN returnCode = handleSet->DeleteStmt.BindStringParam(1, TYPE_COLUMN_SIZE, type, typeSize, L"Bind type to delete");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }

        SQLLEN keySize = sizeof(WCHAR) * key.size();
        returnCode = handleSet->DeleteStmt.BindStringParam(2, KEY_COLUMN_SIZE, key, keySize, L"Bind key to delete");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }

        returnCode = handleSet->DeleteStmt.BindBigIntParam(3, &checkSequenceNumber, L"Bind checkSequenceNumber 1 to delete");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }

        returnCode = handleSet->DeleteStmt.BindBigIntParam(4, &checkSequenceNumber, L"Bind checkSequenceNumber 2 to delete");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }

        SqlStatementExecution sqlStatementExecution(handleSet->DeleteStmt);
        returnCode = sqlStatementExecution.Execute(L"Delete");
        if (SQL_FAILED(returnCode))
        {
            return ErrorCodeValue::StoreUnexpectedError;
        }
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode SqlStore::UpdateOperationLSN(
    TransactionSPtr const & transaction,
    std::wstring const & type,
    std::wstring const & key,
    __in ::FABRIC_SEQUENCE_NUMBER operationLSN)
{
    UNREFERENCED_PARAMETER(transaction);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(key);
    UNREFERENCED_PARAMETER(operationLSN);

    return ErrorCodeValue::NotImplemented;
}

FILETIME SqlStore::GetStoreUtcFILETIME()
{
    FILETIME filetime = {0};

    shared_ptr<SqlHandleSet> handleSet = sqlContext_->GetHandles();
    if (!handleSet)
    {
        SqlStore::WriteWarning(ActionTrace, "Connection is not active");
        return filetime;
    }

    SqlStatementExecution sqlStatementExecution(handleSet->GetTimeStmt);
    SQLRETURN returnCode = sqlStatementExecution.Execute(L"Execute get time");
    if (SQL_FAILED(returnCode))
    {
        return filetime;
    }

    returnCode = SQLFetch(handleSet->GetTimeStmt.Handle);
    if (SQL_FAILED(returnCode))
    {
        handleSet->GetTimeStmt.TraceFailure(ActionTrace, L"Fetch result from get time result set");
        return filetime;
    }

    TIMESTAMP_STRUCT timestamp;
    SQLLEN size;
    returnCode = SQLGetData(handleSet->GetTimeStmt.Handle, 1, SQL_C_TIMESTAMP, &timestamp, sizeof(TIMESTAMP_STRUCT), &size);
    if (SQL_FAILED(returnCode))
    {
        handleSet->GetTimeStmt.TraceFailure(ActionTrace, L"Get data from get time result set");
        return filetime;
    }

    SYSTEMTIME systemTime;
    systemTime.wDay = (DWORD) timestamp.day;
    systemTime.wHour = (DWORD) timestamp.hour;
    systemTime.wMilliseconds = (WORD) (timestamp.fraction / 1000000);
    systemTime.wMinute = (DWORD) timestamp.minute;
    systemTime.wMonth = (DWORD) timestamp.month;
    systemTime.wSecond = (DWORD) timestamp.second;
    systemTime.wYear = (DWORD) timestamp.year;

    if (!SystemTimeToFileTime(&systemTime, &filetime))
    {
        SqlStore::WriteWarning(ActionTrace, "Failed to convert system time to file time: {0}", ::GetLastError());
        return filetime;
    }

    return filetime;
}

bool SqlStore::ValidateTransaction(ILocalStore::TransactionSPtr const & trans)
{
    if (trans)
    {
        SqlTransaction *sqlTrans = static_cast<SqlTransaction *>(trans.get());
        return sqlTrans->IsActive;
    }
    return false;
}

static WStringLiteral const StoreOwner = L"StoreOwner";

SqlSharedStore::SqlSharedStore(wstring const & connectionString)
    : SqlStore(connectionString), ownerId_(-1)
{
}

SqlSharedStore::~SqlSharedStore()
{
    this->Abort();
}

ErrorCode SqlSharedStore::Initialize(CheckOwnerCallback callback)
{
    ErrorCode openResult = this->Open();
    if (!openResult.IsSuccess())
    {
        return openResult;
    }

    TransactionSPtr transaction;
    openResult = SqlStore::CreateTransaction(transaction);
    if (!openResult.IsSuccess())
    {
        return openResult;
    }

    if (!ValidateTransaction(transaction))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    int64 owner = GetCurrentOwner(transaction);
    if (owner < 0)
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    if (!callback())
    {
        return ErrorCodeValue::NotOwner;
    }

    int64 newOwner = owner + 1;
    wstring type(StoreOwner.begin(), StoreOwner.end());
    ErrorCode error;
    if (owner == 0)
    {
        error = Insert(transaction, type, L"", &newOwner, sizeof(__int64), newOwner);
    }
    else
    {
        error = Update(transaction, type, L"", owner, L"", &newOwner, sizeof(__int64), ILocalStore::OperationNumberUnspecified);
    }

    if (error.IsSuccess())
    {
        error = transaction->Commit();
        if (error.IsSuccess())
        {
            ownerId_ = newOwner;
        }
    }

    return error;
}

Common::ErrorCode SqlSharedStore::CreateTransaction(__out Store::ILocalStore::TransactionSPtr & transactionSPtr)
{
    Common::ErrorCode errCode = SqlStore::CreateTransaction(transactionSPtr);

    if (!errCode.IsSuccess())
    {
        return errCode;
    }

    if (!ValidateTransaction(transactionSPtr))
    {
        return ErrorCodeValue::StoreUnexpectedError;
    }

    if (ValidateTransaction(transactionSPtr))
    {
        int64 owner = GetCurrentOwner(transactionSPtr);
        if (owner != ownerId_)
        {
            static_cast<SqlTransaction*>(transactionSPtr.get())->IsActive = false;
        }
    }

    return errCode;
}

int64 SqlSharedStore::GetCurrentOwner(TransactionSPtr const & transaction)
{
    ILocalStore::EnumerationSPtr entries = CreateKeyEnumeration(transaction, wstring(StoreOwner.begin(), StoreOwner.end()), L"");
    ErrorCode error = entries->MoveNext();
    if (error.ReadValue() == ErrorCodeValue::EnumerationCompleted)
    {
        return 0;
    }

    if (!error.IsSuccess())
    {
        return -1;
    }

    int64 owner = 0;
    entries->CurrentOperationLSN(owner);
    return owner;
}

