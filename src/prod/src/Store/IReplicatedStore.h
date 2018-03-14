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

        virtual bool GetIsActivePrimary() const = 0;

        virtual Common::ErrorCode GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER &) = 0;

    public:

        //
        // Create a transaction with a trace id
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

        //
        // Create a simple transaction with the default isolation level of the IReplicatedStore instance
        //
        virtual Common::ErrorCode CreateSimpleTransaction(
            __out TransactionSPtr & transactionSPtr) = 0;

        //
        // Create a simple transaction with the default isolation level of the IReplicatedStore instance
        //
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

        virtual Common::ErrorCode GetQueryStatus(
            __out Api::IStatefulServiceReplicaStatusResultPtr &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual bool IsThrottleNeeded() const
        {
            return false;
        }

        virtual Common::ErrorCode GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const
        {
            UNREFERENCED_PARAMETER(epoch);

            return Common::ErrorCodeValue::NotImplemented;
        }

        //
        // Services using the replicated store can use this method to update the replicator settings
        //
        virtual Common::ErrorCode UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode UpdateReplicatorSettings(Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        //
        // Services using the replicated store can use this method to enable/disable throttle
        //
        virtual Common::ErrorCode SetThrottleCallback(ThrottleCallback const & throttleCallback)
        {
            UNREFERENCED_PARAMETER(throttleCallback);
            
            return Common::ErrorCodeValue::NotImplemented;
        }
                
        // 
        // Get current logical time
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

        virtual void Test_SetTestHookContext(TestHooks::TestHookContext const &)
        {
            CODING_ASSERT("Test_SetTestHookContext not implemented");
        }
    };
}
