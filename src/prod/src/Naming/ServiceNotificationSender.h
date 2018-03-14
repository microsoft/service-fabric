// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotificationSender 
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(ServiceNotificationSender)

    public:

        typedef std::function<void(ClientIdentityHeader const &)> TargetFaultHandler;

    public:
        
        ServiceNotificationSender(
            std::wstring const & traceId,
            EntreeServiceTransportSPtr const &,
            ClientIdentityHeader const &,
            Common::VersionRangeCollectionSPtr const & initialClientVersions,
            TargetFaultHandler const &,
            Common::ComponentRoot const &);

        __declspec (property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec (property(get=get_IsStopped)) bool IsStopped;
        bool get_IsStopped() const { return isStopped_; }

        __declspec (property(get=get_IsClientSynchronized)) bool IsClientSynchronized;
        bool get_IsClientSynchronized() const { return isClientSynchronized_; }

        void Start();
        void Stop();

        void CreateAndEnqueueNotificationForClientSynchronization(
            Common::ActivityId const &,
            Reliability::GenerationNumber const & cacheGeneration,
            Common::VersionRangeCollectionSPtr && cacheVersions,
            Reliability::MatchedServiceTableEntryMap && matchedPartitions,
            Common::VersionRangeCollectionSPtr && updateVersions);

        void CreateAndEnqueueNotification(
            Common::ActivityId const &,
            Reliability::GenerationNumber const & cacheGeneration,
            Common::VersionRangeCollectionSPtr && cacheVersions,
            Reliability::MatchedServiceTableEntryMap && matchedPartitions,
            Common::VersionRangeCollectionSPtr && updateVersions);

        size_t GetPendingNotificationPagesCount(__out ServiceNotificationPageId & next) const;

    private:
        class NotificationPage;
        class QueuedNotification;
        class SendNotificationPageAsyncOperation;

        typedef std::unique_ptr<QueuedNotification> QueuedNotificationUPtr;
        typedef std::shared_ptr<NotificationPage> NotificationPageSPtr;

        void CreateAndEnqueueNotification(
            Common::ActivityId const &,
            Reliability::GenerationNumber const & cacheGeneration,
            Common::VersionRangeCollectionSPtr && cacheVersions,
            Reliability::MatchedServiceTableEntryMap && matchedPartitions,
            Common::VersionRangeCollectionSPtr && updateVersions,
            bool isForClientSynchronization);

        void SchedulePumpNotifications();
        void PumpNotifications();
        bool OnDequeueComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);
        void SendNotification(QueuedNotificationUPtr &&);
        void OnSendNotificationPageComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);
        void OnTargetFaultNotificationComplete(Common::AsyncOperationSPtr const& operation);
        void WrapForEntreeServiceProxy(Transport::MessageUPtr &);

        bool TryStartSendNotificationPages(std::vector<NotificationPageSPtr> const &, bool isForClientSynchronization);
        void FinishSendNotificationPage(ServiceNotificationPageId const &);
        void CheckSynchronizationCompleteCallerHoldsLock(ServiceNotificationPageId const &);

        bool CanInitializeFullCacheVersions();
        bool CanSendFullCacheVersions(ServiceNotificationPageId const &);

        void RaiseTargetFault();

        Common::VersionRangeCollectionSPtr GetEstimatedClientVersions() const;
        void UpdateEstimatedClientVersions(ServiceNotificationPageId const &, Common::VersionRangeCollectionSPtr const &);

        std::wstring traceId_;
        EntreeServiceTransportSPtr transport_;
        ClientIdentityHeader clientIdentity_;
        std::shared_ptr<Common::ReaderQueue<QueuedNotification>> notificationQueue_;
        bool isStopped_;

        std::queue<ServiceNotificationPageId> pendingNotificationPages_;
        std::unordered_set<ServiceNotificationPageId, ServiceNotificationPageId::Hasher> completedNotificationPages_;
        std::unique_ptr<ServiceNotificationPageId> synchronizationPageId_;
        bool isClientSynchronized_;
        mutable Common::RwLock notificationPagesLock_;

        Common::VersionRangeCollectionSPtr estimatedClientVersions_;
        mutable Common::RwLock clientVersionsLock_;

        TargetFaultHandler targetFaultHandler_;
    };

    typedef std::shared_ptr<ServiceNotificationSender> ServiceNotificationSenderSPtr;
}
