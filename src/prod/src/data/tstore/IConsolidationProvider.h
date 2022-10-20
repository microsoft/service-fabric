// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class SnapshotContainer;

        template<typename TKey, typename TValue>
        interface IConsolidationProvider
        {
            K_SHARED_INTERFACE(IConsolidationProvider)

        public:
            __declspec(property(get = get_HasPersistedState)) bool HasPersistedState;
            virtual bool get_HasPersistedState() const = 0;

            __declspec(property(get = get_KeyConverter)) KSharedPtr<Data::StateManager::IStateSerializer<TKey>> KeyConverterSPtr;
            virtual KSharedPtr<Data::StateManager::IStateSerializer<TKey>> get_KeyConverter() const = 0;

            __declspec(property(get = get_ValueConverter)) KSharedPtr<Data::StateManager::IStateSerializer<TValue>> ValueConverterSPtr;
            virtual KSharedPtr<Data::StateManager::IStateSerializer<TValue>> get_ValueConverter() const = 0;

            __declspec(property(get = get_KeyComparer)) KSharedPtr<IComparer<TKey>> KeyComparerSPtr;
            virtual KSharedPtr<IComparer<TKey>> get_KeyComparer() const = 0;

            __declspec(property(get = get_TransactionalReplicator)) TxnReplicator::ITransactionalReplicator::SPtr TransactionalReplicatorSPtr;
            virtual TxnReplicator::ITransactionalReplicator::SPtr get_TransactionalReplicator() const = 0;

            __declspec(property(get = get_LockManager)) KSharedPtr<Data::Utilities::LockManager> LockManagerSPtr;
            virtual KSharedPtr<Data::Utilities::LockManager> get_LockManager() const = 0;

            __declspec(property(get = get_MergeMetadataTable, put = set_MergeMetadataTable)) KSharedPtr<MetadataTable> MergeMetadataTableSPtr;
            virtual KSharedPtr<MetadataTable> get_MergeMetadataTable() const = 0;
            virtual void set_MergeMetadataTable(KSharedPtr<MetadataTable> tableSPtr) = 0;

            __declspec(property(get = get_NextMetadataTable)) KSharedPtr<MetadataTable> NextMetadataTableSPtr;
            virtual KSharedPtr<MetadataTable> get_NextMetadataTable() const = 0;

            __declspec(property(get = get_CurrentMetadataTable)) KSharedPtr<MetadataTable> CurrentMetadataTableSPtr;
            virtual KSharedPtr<MetadataTable> get_CurrentMetadataTable() const = 0;

            __declspec(property(get = get_StateProviderId)) LONG64 StateProviderId;
            virtual LONG64 get_StateProviderId() const = 0;

            __declspec(property(get = get_SnapshotContainer)) KSharedPtr<SnapshotContainer<TKey, TValue>> SnapshotContainerSPtr;
            virtual KSharedPtr<SnapshotContainer<TKey, TValue>> get_SnapshotContainer() const = 0;

            __declspec(property(get = get_IsValueAReferenceType)) bool IsValueAReferenceType;
            virtual bool get_IsValueAReferenceType() const = 0;

            __declspec(property(get = get_WorkingDirectory)) KString::CSPtr WorkingDirectoryCSPtr;
            virtual KString::CSPtr get_WorkingDirectory() const = 0;

            __declspec(property(get = get_EnableBackgroundConsolidation)) bool EnableBackgroundConsolidation;
            virtual bool get_EnableBackgroundConsolidation() const = 0;

            __declspec(property(get = get_EnableSweep)) bool EnableSweep;
            virtual bool get_EnableSweep() const = 0;

            __declspec(property(get = get_MergeHelper)) MergeHelper::SPtr MergeHelperSPtr;
            virtual MergeHelper::SPtr get_MergeHelper() const = 0;

            __declspec(property(get = get_TestDelayOnConsolidation)) ktl::AwaitableCompletionSource<bool>::SPtr TestDelayOnConsolidationSPtr;
            virtual ktl::AwaitableCompletionSource<bool>::SPtr get_TestDelayOnConsolidation() const = 0;
            
            virtual ULONG32 IncrementFileId() = 0;
            virtual ULONG64 IncrementFileStamp() = 0;
            virtual LONG64 CreateTransactionId() = 0;
        };
    }
}
