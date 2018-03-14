// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceResolverTransport;
    class ServiceResolverImpl;
    class LookupVersionStoreProvider;
    typedef Common::EventT<ServiceTableUpdateMessageBody> ServiceUpdateEvent;

    class ServiceResolver : public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
        DENY_COPY(ServiceResolver);

    public:
        ServiceResolver(__in FederationWrapper & transport, Common::ComponentRoot const & root);
        ~ServiceResolver();

        __declspec(property(get=get_Cache)) IServiceTableEntryCache & Cache;
        IServiceTableEntryCache & get_Cache();

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

        Common::AsyncOperationSPtr BeginResolveFMService(
            int64 lookupVersion,
            GenerationNumber const & generation,
            CacheMode::Enum refreshMode,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndResolveFMService(
            Common::AsyncOperationSPtr const & operation,
            __out ServiceTableEntry & fmServiceLocation,
            __out GenerationNumber & generation);

        bool TryGetCachedEntry(
            ConsistencyUnitId const & cuid,
            __out ServiceTableEntry & entry,
            __out GenerationNumber & generation);
        bool TryGetCachedFMEntry(
            __out ServiceTableEntry & entry,
            __out GenerationNumber & generation);

        void ProcessServiceTableUpdateBroadcast(__in Transport::Message & message, Common::ComponentRootSPtr const & root);

        Common::HHandler RegisterBroadcastProcessedEvent(Common::EventHandler handler);
        bool UnRegisterBroadcastProcessedEventEvent(Common::HHandler hHandler);

        Common::EventT<ServiceTableEntry>::HHandler RegisterFMChangeEvent(ServiceUpdateEvent::EventHandler const & handler);
        bool UnRegisterFMChangeEvent(ServiceUpdateEvent::HHandler hhandler);

        void Dispose();

    private:
        friend class ResolveServiceAsyncOperation;

        const std::wstring traceId_;
        Common::Event broadcastProcessedEvent_;
        std::unique_ptr<ServiceResolverTransport> transportToFMM_;
        std::unique_ptr<ServiceResolverTransport> transportToFM_;
        std::unique_ptr<ServiceResolverImpl> resolveWithFMM_;
        std::unique_ptr<ServiceResolverImpl> resolveWithFM_;
        std::shared_ptr<LookupVersionStoreProvider> lookupVersionStateProvider_;
    };
}
