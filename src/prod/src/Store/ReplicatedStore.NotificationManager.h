// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::NotificationManager 
        : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(NotificationManager);

    public:
        NotificationManager(
            __in ReplicatedStore & owner,
            Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
            SecondaryNotificationMode::Enum notificationMode);

        Common::ErrorCode NotifyCopyComplete();

        Common::ErrorCode NotifyReplication(
            std::vector<ReplicationOperation> const &, 
            ::FABRIC_SEQUENCE_NUMBER operationLsn,
            ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked);

        void StartDispatchingNotifications();
        void DrainNotifications();
        void GetQueryStatus(
            __out std::shared_ptr<std::wstring> & copyNotificationPrefix,
            __out size_t & copyNotificationProgress);

    private:
        class IScopedEnumerator;
        class StoreItemMetadata;
        class StoreItem;
        class StoreItemMetadataEnumerator;
        class StoreItemEnumerator;
        class StoreItemListEnumerator;
        class StoreEnumerator;

        typedef std::shared_ptr<StoreItemListEnumerator> StoreItemListEnumeratorSPtr;

        bool IsEnabled() const; 

        bool IsNonBlockingQuorumAcked() const; 

        Common::ErrorCode CreateEnumeration(
            TransactionSPtr const & txSPtr,
            std::wstring const & type,
            std::wstring const & keyPrefix,
            __out EnumerationSPtr & enumSPtr);

        void ScheduleDispatchPump();
        void DispatchPump();
        bool OnDequeueComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        void Dispatch(StoreItemListEnumeratorSPtr const &);

        ReplicatedStore & replicatedStore_;
        Api::ISecondaryEventHandlerPtr const & secondaryEventHandler_;
        SecondaryNotificationMode::Enum notificationMode_;

        std::queue<StoreItemListEnumeratorSPtr> notificationBuffer_;
        RWLOCK(StoreNotificationBuffer, notificationBufferLock_);
        std::shared_ptr<Common::ReaderQueue<StoreItemListEnumeratorSPtr>> dispatchQueueSPtr_;
        Common::ErrorCode dispatchError_;
        Common::ManualResetEvent dispatchDrainedEvent_;

        std::shared_ptr<StoreEnumerator> currentStoreEnumeratorSPtr_;
        Common::RwLock currentStoreEnumeratorLock_;
    };
}
