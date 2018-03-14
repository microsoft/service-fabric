// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;

namespace Reliability
{
    class FMTransport : public ServiceResolverTransport
    {
    public:
        FMTransport(FederationWrapper & transport) : ServiceResolverTransport(transport) {}

        AsyncOperationSPtr BeginRequest(
            MessageUPtr && message,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            return federation_.BeginRequestToFM(move(message), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
        }

        ErrorCode EndRequest(AsyncOperationSPtr const & operation, MessageUPtr & reply)
        {
            return federation_.EndRequestToFM(operation, reply);
        }
    };

    class FMMTransport : public ServiceResolverTransport
    {
    public:
        FMMTransport(FederationWrapper & transport) : ServiceResolverTransport(transport) {}

        AsyncOperationSPtr BeginRequest(
            MessageUPtr && message,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            return federation_.BeginRequestToFMM(move(message), FailoverConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
        }

        ErrorCode EndRequest(AsyncOperationSPtr const & operation, MessageUPtr & reply)
        {
            return federation_.EndRequestToFMM(operation, reply);
        }
    };

    ServiceResolver::ServiceResolver(__in FederationWrapper & transport, ComponentRoot const & root)
        : traceId_(root.TraceId),
        broadcastProcessedEvent_(),
        transportToFMM_(new FMMTransport(transport)),
        transportToFM_(new FMTransport(transport)),
        resolveWithFMM_(make_unique<ServiceResolverImpl>(*Constants::FMMServiceResolverTag, *transportToFMM_, broadcastProcessedEvent_, root)),
        resolveWithFM_(make_unique<ServiceResolverImpl>(*Constants::FMServiceResolverTag, *transportToFM_, broadcastProcessedEvent_, root)),
        lookupVersionStateProvider_(make_shared<LookupVersionStoreProvider>(root.TraceId))
    {
        transport.Federation.Register(lookupVersionStateProvider_);
    }

    IServiceTableEntryCache & ServiceResolver::get_Cache()
    {
        return resolveWithFM_->Cache;
    }

    // Define this to let compiler generate code to call ~ServiceResolverImpl for destructing unique_ptr<ServiceResolverImpl>,
    // so that we do not have to expose the declaration of ServiceResolverImpl to anything but this file 
    ServiceResolver::~ServiceResolver()
    {
    }

    AsyncOperationSPtr ServiceResolver::BeginResolveServicePartition(
        ConsistencyUnitId const & initialNamingServiceCuid,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        vector<VersionedCuid> partition;
        partition.push_back(VersionedCuid(initialNamingServiceCuid));
        return BeginResolveServicePartition(partition, CacheMode::Refresh, activityHeader, timeout, callback, parent);
    }

    AsyncOperationSPtr ServiceResolver::BeginResolveServicePartition(
        vector<VersionedCuid> const & partitions,
        CacheMode::Enum refreshMode,
        FabricActivityHeader const & activityHeader,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ASSERT_IF(
            (partitions.size() == 1) && (partitions[0].Cuid == Constants::FMServiceConsistencyUnitId),
            "{0}:{1}: Should call BeginResolveFMService instead",
            traceId_,
            activityHeader);

        return this->resolveWithFM_->BeginResolveServicePartition(partitions, refreshMode, activityHeader, timeout, callback, parent);
    }

    ErrorCode ServiceResolver::EndResolveServicePartition(
        AsyncOperationSPtr const & operation,
        __out vector<ServiceTableEntry> & serviceLocations,
        __out GenerationNumber & generation)
    {
        return resolveWithFM_->EndResolveServicePartition(operation, serviceLocations, generation);
    }

    AsyncOperationSPtr ServiceResolver::BeginResolveFMService(
        int64 lookupVersion,
        GenerationNumber const & generation,
        CacheMode::Enum refreshMode,
        FabricActivityHeader const & activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        vector<VersionedCuid> partitions;
        partitions.push_back(VersionedCuid(Constants::FMServiceConsistencyUnitId, lookupVersion, generation));
        return this->resolveWithFMM_->BeginResolveServicePartition(partitions, refreshMode, activityHeader, timeout, callback, parent);
    }

    ErrorCode ServiceResolver::EndResolveFMService(
        AsyncOperationSPtr const & operation, 
        __out ServiceTableEntry & fmServiceLocation,
        __out GenerationNumber & generation)
    {
        vector<ServiceTableEntry> serviceLocations;
        auto errorCode = resolveWithFMM_->EndResolveServicePartition(operation, serviceLocations, generation);
        if (!errorCode.IsSuccess())
        {
            return errorCode;
        }

        ASSERT_IFNOT(
            (serviceLocations.size() == 1) &&
                (serviceLocations[0].ConsistencyUnitId == Constants::FMServiceConsistencyUnitId),
            "ServiceTableUpdate from FMM should contain exactly one entry of FM service");

        fmServiceLocation = serviceLocations[0];
        return errorCode;
    }

    bool ServiceResolver::TryGetCachedFMEntry(
        __out ServiceTableEntry & entry,
        __out GenerationNumber & generation)
    {
        return resolveWithFMM_->TryGetCachedEntry(Constants::FMServiceConsistencyUnitId, entry, generation);
    }

    bool ServiceResolver::TryGetCachedEntry(
        ConsistencyUnitId const & cuid, 
        __out ServiceTableEntry & entry,
        __out GenerationNumber & generation)
    {
        ASSERT_IF(cuid == Constants::FMServiceConsistencyUnitId, "should call TryGetCachedFMEntry instead");
        return resolveWithFM_->TryGetCachedEntry(cuid, entry, generation);
    }

    void ServiceResolver::ProcessServiceTableUpdateBroadcast(__in Message & message, ComponentRootSPtr const & root)
    {
        ServiceTableUpdateMessageBody body;
        if (!message.GetBody(body))
        {
            WriteWarning(
                Constants::ServiceResolverSource,
                traceId_,
                "Received invalid message id = {0}. Error={1:x}", 
                message.MessageId, 
                message.Status);
            return;
        }

        FabricActivityHeader activityHeader;
        WriteNoise(
            Constants::ServiceResolverSource,
            traceId_,
            "{0}: Storing broadcasted updates: IsFromFMM={1}, Generation={2}, VersionRangeCollection={3}, entry number={4}",
            activityHeader,
            body.IsFromFMM,
            body.Generation,
            body.VersionRangeCollection,
            body.ServiceTableEntries.size());

        if (body.IsFromFMM)
        {
            ASSERT_IFNOT(
                (body.ServiceTableEntries.size() == 0) ||
                ((body.ServiceTableEntries.size() == 1) &&
                    (body.ServiceTableEntries[0].ConsistencyUnitId == Constants::FMServiceConsistencyUnitId)),
                    "{0}: ServiceTableUpdate from FMM should contain exactly one entry of FM service",
                    activityHeader);

            this->resolveWithFMM_->ProcessServiceTableUpdate(body, activityHeader, root);
            this->lookupVersionStateProvider_->UpdateFMMState(body.Generation, body.EndVersion);
        }
        else
        {
            this->resolveWithFM_->ProcessServiceTableUpdate(body, activityHeader, root);
            this->lookupVersionStateProvider_->UpdateFMState(body.Generation, body.EndVersion);
        }
    }

    HHandler ServiceResolver::RegisterBroadcastProcessedEvent(EventHandler handler)
    {
        return broadcastProcessedEvent_.Add(handler);
    }

    bool ServiceResolver::UnRegisterBroadcastProcessedEventEvent(HHandler hHandler)
    {
        return broadcastProcessedEvent_.Remove(hHandler);
    }

    EventT<ServiceTableEntry>::HHandler ServiceResolver::RegisterFMChangeEvent(ServiceUpdateEvent::EventHandler const & handler)
    {
        return resolveWithFMM_->RegisterFMChangeEvent(handler);
    }

    bool ServiceResolver::UnRegisterFMChangeEvent(ServiceUpdateEvent::HHandler hhandler)
    {
        return resolveWithFMM_->UnRegisterFMChangeEvent(hhandler);
    }

    void ServiceResolver::Dispose()
    {
        resolveWithFMM_->Dispose();
        resolveWithFM_->Dispose();
    }
}
