// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceResolverTransport
    {
        DENY_COPY(ServiceResolverTransport)

    public:
        ServiceResolverTransport(__in FederationWrapper & federation) : federation_(federation) {}
        virtual ~ServiceResolverTransport() = 0 {} ;

        virtual Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && message,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndRequest(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) = 0;

    protected:
        FederationWrapper & federation_;
    };

    class ServiceResolverImpl : public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
        DENY_COPY(ServiceResolverImpl);

    public:
        ServiceResolverImpl(
            std::wstring const & tag,
            __in ServiceResolverTransport & transport, 
            __in Common::Event & broadcastProcessedEvent,
            Common::ComponentRoot const & root);

        __declspec(property(get=get_Cache)) IServiceTableEntryCache & Cache;
        IServiceTableEntryCache & get_Cache() { return cache_; }

        Common::AsyncOperationSPtr BeginResolveServicePartition(
            ConsistencyUnitId const & initialNamingServiceCuid,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginResolveServicePartition(
            std::vector<VersionedCuid> const & partitions,
            CacheMode::Enum refreshMode,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<ServiceTableEntry> & serviceLocations,
            __out GenerationNumber & generation);

        bool TryGetCachedEntry(
            ConsistencyUnitId const & cuid,
            __out ServiceTableEntry & entry, 
            __out GenerationNumber & generation);

        void ProcessServiceTableUpdate(
            ServiceTableUpdateMessageBody const & body,
            Transport::FabricActivityHeader const & activityHeader,
            Common::ComponentRootSPtr const & root);

        Common::EventT<ServiceTableEntry>::HHandler RegisterFMChangeEvent(ServiceUpdateEvent::EventHandler const & handler);
        bool UnRegisterFMChangeEvent(ServiceUpdateEvent::HHandler hhandler);

        void Dispose();

    private:
        class ResolveServiceAsyncOperation;
        class LookupRequestLinkableAsyncOperation;

        void RefreshCache(
            Transport::FabricActivityHeader const & activityHeader, 
            Common::AsyncOperationSPtr const & root);
        void OnRefreshComplete(Common::AsyncOperationSPtr const & refreshOperation, bool expectedCompletedSynchronously);

        const std::wstring traceId_;
        // The tag is a global string, so it's ok to keep it by reference
        std::wstring const & tag_;
        ServiceResolverTransport & transport_;
        Common::Event & broadcastProcessedEvent_;
        ServiceResolverCache cache_;
        Common::AsyncOperationSPtr primaryPendingLookupRequestOperation_;
        Common::RwLock pendingCommunicationLock_;
        LONG pendingCommunicationOperationCount_;
        Common::TimerSPtr refreshTimer_;
        ServiceUpdateEvent fmChangeEvent_;
        Common::atomic_bool isProcessingMissedBroadcast_;

        bool isDisposed_;
    };
}
