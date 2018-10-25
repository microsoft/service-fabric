// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IStoreBase.h"

namespace Store
{
    class IReplicatedStore 
        : public Api::IStatefulServiceReplica
        , public IStoreBase
    {
    public:

        //
        // IStatefulServiceReplica
        //
        // Open, ChangeRole, Close, Abort are implemented 
        // by concrete classes: ReplicatedStore, TSReplicatedStore
        //

        virtual Common::ErrorCode GetQueryStatus(__out Api::IStatefulServiceReplicaStatusResultPtr &) override
        {
            Common::Assert::TestAssert("GetQueryStatus not implemented");
            return Common::ErrorCodeValue::NotImplemented;
        }

        Common::ErrorCode UpdateInitializationData(std::vector<byte> &&)
        {
            Common::Assert::TestAssert("UpdateInitializationData not implemented");
            return Common::ErrorCodeValue::NotImplemented;
        }

    public:

        //
        // IReplicatedStore
        //

        virtual Common::ErrorCode CreateTransaction(
            __in Common::ActivityId const & activityId,
            __out TransactionSPtr & transactionSPtr)
        {
            UNREFERENCED_PARAMETER(activityId);
            return static_cast<Store::IStoreBase *>(this)->CreateTransaction(transactionSPtr);
        }

        virtual Common::ErrorCode CreateTransaction(
            __in Common::ActivityId const & activityId,
            TransactionSettings const &,
            __out TransactionSPtr & transactionSPtr)
        {
            return this->CreateTransaction(activityId, transactionSPtr);
        }

        virtual Common::ErrorCode CreateSimpleTransaction(__out TransactionSPtr & transactionSPtr) = 0;

        virtual Common::ErrorCode CreateSimpleTransaction(
            __in Common::ActivityId const & activityId,
            __out TransactionSPtr & transactionSPtr)
        {
            UNREFERENCED_PARAMETER(activityId);
            return CreateSimpleTransaction(transactionSPtr);
        }

        // Create a simple transaction with the specific isolation level.
        // A simple transaction is one which is known not to conflict with
        // other simple transactions that are open currently.
        // Simple transactions can be grouped into a single store transaction
        // since they are known not to conflict with each other.
        //
        virtual Common::ErrorCode CreateSimpleTransaction(
            __in ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            __out TransactionSPtr & transactionSPtr)
        {
            return GetDefaultIsolationLevel() == isolationLevel ? CreateSimpleTransaction(transactionSPtr) : Common::ErrorCodeValue::InvalidOperation;
        }

        // Used for KVS/ESE logical rebuild and KeyValueStoreMigrator database migration
        //
        virtual Common::ErrorCode CreateEnumerationByPrimaryIndex(
            ILocalStoreUPtr const &, 
            std::wstring const & typeStart,
            std::wstring const & keyStart,
            __out TransactionSPtr &,
            __out EnumerationSPtr &)
        {
            UNREFERENCED_PARAMETER(typeStart);
            UNREFERENCED_PARAMETER(keyStart);

            return Common::ErrorCodeValue::NotImplemented;
        }

        // Inserts a new item.
        //
        // { type, key } tuple must be unique.
        // value: must be non-null, but valueSizeInBytes can be 0 to insert an empty value.
        // sequenceNumber: must be one of the following:
        // - a value >0:  This becomes the new sequence number.
        // - SequenceNumberAuto: Requests that the store generate and write a new sequence number.
        // - SequenceNumberDeferred: Avoids writing the sequence number now because it will be updated
        //   later during this transaction.
        virtual Common::ErrorCode Insert(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in void const * value,
            __in size_t valueSizeInBytes) = 0;

        // Updates an existing row.
        //
        // checkSequenceNumber: If not 0, the update only occurs if the existing sequenceNumber
        //   matches checkSequenceNumber exactly.
        // newKey: if non-empty, updates the key for this row.
        // value:  if non-NULL, updates the value for this row.
        // sequenceNumber: must be one of the following:
        // - a value >0:  This becomes the new sequence number.
        // - SequenceNumberAuto: Requests that the store generate and write a new sequence number.
        // - SequenceNumberDeferred: Avoids writing the sequence number now because it will be updated
        //   later during this transaction.
        virtual Common::ErrorCode Update(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber,
            __in std::wstring const & newKey,
            __in_opt void const * newValue,
            __in size_t valueSizeInBytes) = 0;

        // Deletes an existing row.
        // checkSequenceNumber: If not 0, the update only occurs if the existing sequenceNumber
        //   matches checkSequenceNumber exactly.
        virtual Common::ErrorCode Delete(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber = ILocalStore::SequenceNumberIgnore) = 0;

        /// <summary>
        /// Creates a full backup of the replicated store to the specified directory.
        /// </summary>
        /// <param name="dir">The destination directory where the store should be backed up to.</param>
        /// <returns>ErrorCode::Success() if the backup was successfully done. An appropriate ErrorCode otherwise.</returns>
        virtual Common::ErrorCode BackupLocal(std::wstring const & dir)
        {
            UNREFERENCED_PARAMETER(dir);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode RestoreLocal(std::wstring const & dir)
        {
            UNREFERENCED_PARAMETER(dir);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::AsyncOperationSPtr BeginBackupLocal(
            std::wstring const & backupDirectory,
            StoreBackupOption::Enum,
            Api::IStorePostBackupHandlerPtr const &,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &,
            Common::ErrorCode createError = Common::ErrorCodeValue::Success)
        {
            UNREFERENCED_PARAMETER(backupDirectory);

            return nullptr;
        }

        virtual Common::ErrorCode EndBackupLocal(
            __in Common::AsyncOperationSPtr const & operation)
        {
            UNREFERENCED_PARAMETER(operation);

            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::AsyncOperationSPtr BeginRestoreLocal(
            __in std::wstring const & backupDirectory,
            RestoreSettings const &,
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &)
        {
            UNREFERENCED_PARAMETER(backupDirectory);

            return nullptr;
        }

        virtual Common::ErrorCode EndRestoreLocal(
            __in Common::AsyncOperationSPtr const & operation)
        {
            UNREFERENCED_PARAMETER(operation);

            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual bool GetIsActivePrimary() const = 0;

        virtual Common::ErrorCode GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER &) = 0;

        virtual Common::ErrorCode GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const
        {
            UNREFERENCED_PARAMETER(epoch);

            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode UpdateReplicatorSettings(Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        //
        // Commit throttling is a feature currently only used by HM
        //
        virtual Common::ErrorCode SetThrottleCallback(ThrottleCallback const & throttleCallback)
        {
            UNREFERENCED_PARAMETER(throttleCallback);
            
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual bool IsThrottleNeeded() const { return false; }
                
        // 
        // Replicated logical time is currently not being used by any components.
        // However, the same concept and design of logical time is used
        // by the KVS and volatile actor state providers (see VolatileLogicalTimeManager.cs).
        //  
        virtual Common::ErrorCode GetCurrentStoreTime(__out int64 &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }
        
        virtual Common::ErrorCode InitializeRepairPolicy(
            Api::IClientFactoryPtr const &, 
            std::wstring const &,
            bool allowRepairUpToQuorum)
        {
            UNREFERENCED_PARAMETER(allowRepairUpToQuorum);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode InitializeLocalStoreForUnittests(bool databaseShouldExist = false)
        {
            UNREFERENCED_PARAMETER(databaseShouldExist);
            return Common::ErrorCodeValue::NotImplemented;
        };

        virtual bool IsMigrationSourceSupported() { return false; }

        virtual bool ShouldMigrateKey(std::wstring const & type, std::wstring const & key)
        {
            UNREFERENCED_PARAMETER(type);
            UNREFERENCED_PARAMETER(key);
            Common::Assert::CodingError("ShouldMigrateKey not implemented");
        }

        virtual void SetQueryStatusDetails(std::wstring const &)
        {
            Common::Assert::CodingError("SetQueryStatusDetails not implemented");
        }

        virtual void SetMigrationQueryResult(std::unique_ptr<MigrationQueryResult> &&)
        {
            Common::Assert::CodingError("SetMigrationQueryResult not implemented");
        }

        virtual void SetTxEventHandler(IReplicatedStoreTxEventHandlerWPtr const &)
        {
            Common::Assert::CodingError("SetTxEventHandler not overridden");
        }

        virtual void AbortOutstandingTransactions()
        {
            Common::Assert::CodingError("AbortOutstandingTransactions not overridden");
        }

        virtual void ClearTxEventHandlerAndBlockWrites()
        {
            Common::Assert::CodingError("ClearTxEventHandlerAndBlockWrites not overridden");
        }

        virtual void Test_SetTestHookContext(TestHooks::TestHookContext const &)
        {
            Common::Assert::TestAssert("Test_SetTestHookContext not implemented");
        }
    };
}
