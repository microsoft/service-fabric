// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sql.h>

namespace Store
{
    class SqlStore
        : public ILocalStore
        , public std::enable_shared_from_this<SqlStore>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::SqlStore>
    {
        DENY_COPY(SqlStore)

      public:
        typedef std::shared_ptr<SqlStore> SPtr;
        typedef std::weak_ptr<SqlStore> WPtr;

        static SPtr Create(std::wstring const & connectionString);

        // Don't call contructor directly, always call Create() above,
        // This constructor is made public only for make_shared
        SqlStore(std::wstring const & connectionString);

        virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel();

        virtual Common::ErrorCode CreateTransaction(
            __out TransactionSPtr & transactionSPtr);

        virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode CreateEnumerationByOperationLSN(
            __in TransactionSPtr const & transaction,
            __in _int64 fromOperationNumber,
            __out EnumerationSPtr & enumerationSPtr);

        virtual Common::ErrorCode Insert(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in void const * value,
            size_t valueSizeInBytes,
            __int64 sequenceNumber = ILocalStore::SequenceNumberIgnore);

        virtual Common::ErrorCode Update(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __int64 checkSequenceNumber,
            std::wstring const & newKey,
            __in_opt void const * newValue,
            size_t valueSizeInBytes,
            __int64 sequenceNumber = ILocalStore::SequenceNumberIgnore);

        // If key is null it will delete all the rows of the given type.
        virtual Common::ErrorCode Delete(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __int64 checkSequenceNumber = 0);

        virtual Common::ErrorCode UpdateOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in ::FABRIC_SEQUENCE_NUMBER operationLSN);

        virtual FILETIME GetStoreUtcFILETIME();

        virtual Common::ErrorCode GetLastChangeOperationLSN(
            __in TransactionSPtr const & transaction,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

        bool ValidateTransaction(ILocalStore::TransactionSPtr const & trans);

        EnumerationSPtr CreateTypeEnumeration(
            TransactionSPtr const & transaction,
            std::wstring const & type);

        EnumerationSPtr CreateKeyEnumeration(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key);

        class SqlEnumeration;
        class SqlTransaction;

        RWLOCK(StoreSqlStoreTransaction, transactionLock_);

    private: 
        Common::ErrorCode InitializeCallerHoldingLock();
        void ScheduleRecoveryIfNeededCallerHoldingLock();
        bool InitializeStatementsCallerHoldingLock();
        bool EnsureConnectionCallerHoldingLock();

        bool IsTableMissing();

        class SqlContext;
        std::shared_ptr<SqlContext> sqlContext_;

        class SqlHandle;
        class SqlEnvironment;
        class SqlConnection;
        class SqlStatement;
        class SqlStatementExecution;
        class SqlHandleSet;
    };

    typedef std::function<bool(void)> CheckOwnerCallback;

    class SqlSharedStore : SqlStore
    {
    public:
        SqlSharedStore(std::wstring const & connectionString);
        ~SqlSharedStore();

        Common::ErrorCode Initialize(CheckOwnerCallback callback);

        virtual Common::ErrorCode CreateTransaction(
            __out TransactionSPtr & transactionSPtr);

        int64 GetCurrentOwner(TransactionSPtr const & transaction);

    private:
        int64 ownerId_;
    };
}
