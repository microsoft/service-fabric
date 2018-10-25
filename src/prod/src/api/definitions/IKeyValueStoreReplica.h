// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreReplica : public IStatefulServiceReplica
    {
    public:
        virtual ~IKeyValueStoreReplica() {};

        virtual Common::ErrorCode GetCurrentEpoch(
            __out ::FABRIC_EPOCH & currentEpoch) = 0;

        virtual Common::ErrorCode UpdateReplicatorSettings(
            ::FABRIC_REPLICATOR_SETTINGS const & replicatorSettings) = 0;

        virtual Common::ErrorCode CreateTransaction(
            ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            __out ITransactionPtr & transaction) = 0;

        virtual Common::ErrorCode CreateTransaction(
            ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS const & settings,
            __out ITransactionPtr & transaction) = 0;

        virtual Common::ErrorCode Add(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value) = 0;

        virtual Common::ErrorCode Remove(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber) = 0;

        virtual Common::ErrorCode Update(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber) = 0;

        virtual Common::ErrorCode Get(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out IKeyValueStoreItemResultPtr & itemResult) = 0;

        virtual Common::ErrorCode TryAdd(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            __out bool & added) = 0;

        virtual Common::ErrorCode TryRemove(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            __out bool & exists) = 0;

        virtual Common::ErrorCode TryUpdate(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            LONG valueSizeInBytes,
            const BYTE * value,
            ::FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            __out bool & exists) = 0;

        virtual Common::ErrorCode TryGet(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out IKeyValueStoreItemResultPtr & itemResult) = 0;

        virtual Common::ErrorCode GetMetadata(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out IKeyValueStoreItemMetadataResultPtr & itemMetadataResult) = 0;

        virtual Common::ErrorCode TryGetMetadata(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out IKeyValueStoreItemMetadataResultPtr & itemMetadataResult) = 0;

        virtual Common::ErrorCode Contains(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            __out BOOLEAN & result) = 0;

        virtual Common::ErrorCode Enumerate(
            ITransactionBasePtr const & transaction,
            __out IKeyValueStoreItemEnumeratorPtr & itemEnumerator) = 0;

        virtual Common::ErrorCode EnumerateByKey(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            bool strictPrefix,
            __out IKeyValueStoreItemEnumeratorPtr & itemEnumerator) = 0;

        virtual Common::ErrorCode EnumerateMetadata(
            ITransactionBasePtr const & transaction,
            __out IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) = 0;

        virtual Common::ErrorCode EnumerateMetadataByKey(
            ITransactionBasePtr const & transaction,
            std::wstring const & key,
            bool strictPrefix,
            __out IKeyValueStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) = 0;

        virtual Common::ErrorCode Backup(
            std::wstring const & dir) = 0;

        virtual Common::AsyncOperationSPtr BeginBackup(
            __in std::wstring const & backupDirectory,
            __in FABRIC_STORE_BACKUP_OPTION backupOption,
            __in IStorePostBackupHandlerPtr const & postBackupHandler,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::ErrorCode Restore(
            std::wstring const & dir) = 0;

        virtual Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const & backupDirectory,
            Store::RestoreSettings const &,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndRestore(
            __in Common::AsyncOperationSPtr const & operation) = 0;
    };
}
