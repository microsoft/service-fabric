// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IStoreBase.h"
#include "StoreBackupOption.h"

namespace Store
{
    class ILocalStore : public IStoreBase
    {
    public:
        virtual ~ILocalStore() { }

        //
        // OperationNumberUnspecified is used to indicate caller is not requesting the operation number
        // to be set to a specific value as part of the DML operation.
        //
        static const _int64 OperationNumberUnspecified = -1;
        //
        // SequenceNumberIgnore is used to indicate no optimistic concurrency check
        // should to be performed.
        //
        static const _int64 SequenceNumberIgnore = 0;
                
        virtual bool StoreExists() { return false; };

        virtual Common::ErrorCode Initialize(std::wstring const &) { return Common::ErrorCodeValue::Success; };

        virtual Common::ErrorCode Initialize(std::wstring const & instanceName, Federation::NodeId const & nodeId) 
        { 
            return this->Initialize(Common::wformatString("{0}-{1}", instanceName, nodeId)); 
        };

        virtual Common::ErrorCode Initialize(std::wstring const &, Common::Guid const &, FABRIC_REPLICA_ID, FABRIC_EPOCH const &)
        { 
            return Common::ErrorCodeValue::NotImplemented;
        };

        virtual Common::ErrorCode MarkCleanupInTerminate() { return Common::ErrorCodeValue::NotImplemented; };
        virtual Common::ErrorCode Terminate() { return Common::ErrorCodeValue::Success; };
        virtual void Drain() { };
        virtual Common::ErrorCode Cleanup() { return Common::ErrorCodeValue::Success; };

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
            __in size_t valueSizeInBytes,
            __in _int64 operationNumber = ILocalStore::OperationNumberUnspecified,
            __in FILETIME const * lastModifiedOnPrimaryUtc = nullptr) = 0;

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
            __in size_t valueSizeInBytes,
            __in _int64 operationNumber = ILocalStore::OperationNumberUnspecified,
            __in FILETIME const * lastModifiedOnPrimaryUtc = nullptr) = 0;

        // Deletes an existing row.
        // checkSequenceNumber: If not 0, the update only occurs if the existing sequenceNumber
        //   matches checkSequenceNumber exactly.
        virtual Common::ErrorCode Delete(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __in _int64 checkOperationNumber = ILocalStore::OperationNumberUnspecified) = 0;

        virtual Common::ErrorCode GetOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
        {
            UNREFERENCED_PARAMETER(transaction);
            UNREFERENCED_PARAMETER(type);
            UNREFERENCED_PARAMETER(key);
            UNREFERENCED_PARAMETER(operationLSN);

            return Common::ErrorCodeValue::NotImplemented;
        }

#if defined(PLATFORM_UNIX)
        virtual Common::ErrorCode Lock(
                TransactionSPtr const & transaction,
                std::wstring const & type,
                std::wstring const & key)
        {
            UNREFERENCED_PARAMETER(transaction);
            UNREFERENCED_PARAMETER(type);
            UNREFERENCED_PARAMETER(key);

            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode Flush()
        {
            return Common::ErrorCodeValue::NotImplemented;
        }
#endif

        // Updates the operationLSN of a row when replication completes with quorum ack
        // All rows which were modified by the transaction are called with this before
        // local commit of the transaction is issued
        virtual Common::ErrorCode UpdateOperationLSN(
            TransactionSPtr const & transaction,
            std::wstring const & type,
            std::wstring const & key,
            __in ::FABRIC_SEQUENCE_NUMBER operationLSN) = 0;

        virtual Common::ErrorCode CreateEnumerationByOperationLSN(
            __in TransactionSPtr const & transaction,
            __in _int64 fromOperationNumber,
            __out EnumerationSPtr & enumerationSPtr) = 0;

        virtual Common::ErrorCode GetLastChangeOperationLSN(
            __in TransactionSPtr const & transaction,
            __out ::FABRIC_SEQUENCE_NUMBER & operationLSN) = 0;

        /// <summary>
        /// Backs up the local store to the specified directory.
        /// </summary>
        /// <param name="dir">The destination directory where the current local store should be backed up to.</param>
		/// <param name="backupOption">The backup option. The default option is StoreBackupOption::Full.</param>
        /// <returns>ErrorCode::Success() if the backup was successfully done. An appropriate ErrorCode otherwise.</returns>
		virtual Common::ErrorCode Backup(std::wstring const & dir, StoreBackupOption::Enum backupOption = StoreBackupOption::Full)
        { 
            UNREFERENCED_PARAMETER(dir);
            UNREFERENCED_PARAMETER(backupOption);
            return Common::ErrorCodeValue::NotImplemented; 
        }

        virtual Common::ErrorCode PrepareRestoreForValidation(
            std::wstring const & dir, 
            std::wstring const & instanceName)
        { 
            UNREFERENCED_PARAMETER(dir);
            UNREFERENCED_PARAMETER(instanceName);
            return Common::ErrorCodeValue::NotImplemented; 
        }

        // This is invoked by replicated store during restore. Merge the backup chain (one full and 
        // zero or more incremental backup) into a single folder (preferably into full backup folder
        // to avoid extra copying) and return merged directory. Each local store have its own implementation
        // specific backup directory structure for backup and should override this function.
        virtual Common::ErrorCode MergeBackupChain(
            std::map<ULONG, std::wstring> const & backupChainOrderedDirList,
            __out std::wstring & mergedBackupDir)
        {
            UNREFERENCED_PARAMETER(backupChainOrderedDirList);
            UNREFERENCED_PARAMETER(mergedBackupDir);

            return Common::ErrorCodeValue::NotImplemented;
        }

        // This is invoked by replicated store to enable automatic log truncation.
        // E.g. EseLocalStore requires log truncation when it has incremental backup enabled.
        virtual bool IsLogTruncationRequired()
        {
            return false;
        }

		virtual bool IsIncrementalBackupEnabled()
		{
			return false;
		}

        virtual Common::ErrorCode PrepareRestoreFromValidation()
        { 
            return Common::ErrorCodeValue::NotImplemented; 
        }

        virtual Common::ErrorCode PrepareRestore(std::wstring const & dir)
        { 
            UNREFERENCED_PARAMETER(dir);
            return Common::ErrorCodeValue::NotImplemented; 
        }
        
        //
        // Query
        //

        virtual Common::ErrorCode EstimateRowCount(__out size_t &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode EstimateDbSizeBytes(__out size_t &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }
    };
}
