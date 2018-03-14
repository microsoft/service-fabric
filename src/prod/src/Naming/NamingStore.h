// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingStore
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>
    {
        DENY_COPY(NamingStore);

    public:
        NamingStore(
            __in Store::IReplicatedStore &, 
            __in StoreServiceProperties & properties, 
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId);

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID const & ReplicaId;
        FABRIC_REPLICA_ID const & get_ReplicaId() const { return replicaId_; }

        bool IsRepairable(Common::ErrorCode const & error);

        bool IsInvalidRequest(Common::ErrorCode const & error);

        bool IsCancellable(Common::ErrorCode const & error);
        
        bool NeedsReversion(Common::ErrorCode const & error);

        Common::ErrorCode ToRemoteError(Common::ErrorCode && error);

        // ***************************
        // Replicated store operations
        // ***************************

        Common::ErrorCode CreateTransaction(Transport::FabricActivityHeader const &, TransactionSPtr &);

        Common::ErrorCode CreateEnumeration(
            TransactionSPtr const &,
            std::wstring const & storeType,
            std::wstring const & storeName,
            EnumerationSPtr & enumSPtr);

        Common::ErrorCode DeleteFromStore(
            TransactionSPtr const &,
            std::wstring const & storeType,
            std::wstring storeName,
            _int64 currentSequenceNumber);

        Common::ErrorCode TrySeekToDataItem(
            std::wstring const & type, 
            std::wstring const & key, 
            EnumerationSPtr const &);

        Common::ErrorCode TryGetCurrentSequenceNumber(
            TransactionSPtr const &,
            std::wstring const & type,
            std::wstring const & key,
            __out _int64 &);

        Common::ErrorCode TryGetCurrentSequenceNumber(
            TransactionSPtr const &,
            std::wstring const & type, 
            std::wstring const & key,
            __out _int64 &,
            __out EnumerationSPtr &);                    

        Common::ErrorCode GetCurrentProgress(__out _int64 &);

        template <class T>
        Common::ErrorCode TryWriteData(
            TransactionSPtr const & txSPtr,
            T const & data,
            std::wstring const & type,
            std::wstring const & key,
            _int64 currentSequenceNumber = Store::ILocalStore::OperationNumberUnspecified);

        template <class T>
        Common::ErrorCode TryWriteDataAndGetCurrentSequenceNumber(
            TransactionSPtr const & txSPtr,
            T const & data,
            std::wstring const & type,
            std::wstring const & key,
            __inout _int64 & currentSequenceNumber);

        template <class T>
        Common::ErrorCode TryReadData(
            TransactionSPtr const & txSPtr,
            std::wstring const & type,
            std::wstring const & key,
            __out T & data,
            __out _int64 & currentSequenceNumber);

        template <class T>
        Common::ErrorCode ReadCurrentData(EnumerationSPtr const & enumSPtr, __out T & result);

        static Common::AsyncOperationSPtr BeginCommit(
            TransactionSPtr &&, 
            Common::TimeSpan const &, 
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        static Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const &);
        static Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const &, __out FABRIC_SEQUENCE_NUMBER &);

        static void CommitReadOnly(TransactionSPtr &&);

        // ***************
        // Name operations
        // ***************
        
        template <class T>
        Common::ErrorCode TryInsertRootName(TransactionSPtr const &, T nameData, std::wstring const & dataType);

        Common::AsyncOperationSPtr BeginAcquireNamedLock(
            Common::NamingUri const & name,
            std::wstring const & traceId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation);
        Common::ErrorCode EndAcquireNamedLock(
            Common::NamingUri const & name, 
            std::wstring const & traceId,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginAcquireDeleteServiceNamedLock(
            Common::NamingUri const & name,
            bool const isForceDelete,
            std::wstring const & traceId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation);
        Common::ErrorCode EndAcquireDeleteServiceNamedLock(
            Common::NamingUri const & name,
            std::wstring const & traceId,
            Common::AsyncOperationSPtr const & operation);

        void ReleaseNamedLock(
            Common::NamingUri const & name, 
            std::wstring const & traceId,
            Common::AsyncOperationSPtr const & operation);

        void ReleaseDeleteServiceNamedLock(
            Common::NamingUri const & name,
            std::wstring const & traceId,
            Common::AsyncOperationSPtr const & operation);

        enum class NotificationEnum
        {
            ConvertToForceDelete,
        };

        bool ShouldForceDelete(
            Common::NamingUri const & name);

        // *******************
        // Property operations
        // *******************

        Common::ErrorCode ReadProperty(
            Common::NamingUri const &,
            TransactionSPtr const & txSPtr,
            NamePropertyOperationType::Enum,
            std::wstring const & storeType,
            std::wstring const & storeKey,
            size_t operationIndex,
            __out NamePropertyResult & property);

        Common::ErrorCode ReadProperty(
            Common::NamingUri const &,
            EnumerationSPtr const & enumSPtr,
            NamePropertyOperationType::Enum,
            size_t operationIndex,
            __out NamePropertyResult & property);

    private:
        class CommitAsyncOperation;

        Common::AsyncOperationSPtr BeginAcquireNamedLockHelper(
            Common::NamingUri const & name,
            bool const isForceDelete,
            std::wstring const & traceId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation);
            
        Store::IReplicatedStore & iReplicatedStore_;
        StoreServiceProperties & properties_;
        std::wstring traceId_;
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        std::map<Common::NamingUri, Common::AsyncExclusiveLockSPtr> operationQueues_;
        Common::RwLock operationQueueLock_;

        typedef std::unordered_map<
            Common::NamingUri,
            NotificationEnum,
            Common::NamingUri::Hasher,
            Common::NamingUri::Hasher> NotificationMap;
        NotificationMap operationMsgNotifier_;
        // Guarding operationMsgNotifier_. If operationQueueLock_ is also acquired meanwhile, make sure to take operationQueueLock_ before this.
        Common::RwLock operationMsgNotifierLock_;
    };

    typedef std::unique_ptr<NamingStore> NamingStoreUPtr;
}
