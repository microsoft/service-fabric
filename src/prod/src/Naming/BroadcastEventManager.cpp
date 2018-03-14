// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;

    StringLiteral const TraceComponent("BroadcastEventManager");
    
    BroadcastEventManager::BroadcastEventManager(
        __in ServiceResolver & resolver, 
        __in GatewayPsdCache & psdCache,
        GatewayEventSource const & trace,
        wstring const & gatewayTraceId)
      : FabricComponent()
      , pendingLocationNotificationRequests_(trace, gatewayTraceId)
      , resolver_(resolver)
      , psdCache_(psdCache)
      , trace_(trace)
      , gatewayTraceId_(gatewayTraceId)
    {
    }

    ErrorCode BroadcastEventManager::OnOpen()
    {
        broadcastListenerHandle_ = resolver_.RegisterBroadcastProcessedEvent([this](EventArgs const & args)
        {
            this->OnBroadcastProcessed(args);
        });

        return ErrorCodeValue::Success;
    }

    ErrorCode BroadcastEventManager::OnClose()
    {
        Cleanup();
        return ErrorCodeValue::Success;
    }

    void BroadcastEventManager::OnAbort()
    {
        Cleanup();
    }

    void BroadcastEventManager::Cleanup()
    {
        resolver_.UnRegisterBroadcastProcessedEventEvent(broadcastListenerHandle_);
        pendingLocationNotificationRequests_.Close();
    }

    ErrorCode BroadcastEventManager::AddRequests(
        AsyncOperationSPtr const & owner,
        OnBroadcastReceivedCallback const & callback,
        BroadcastRequestVector && requests)
    {
        return pendingLocationNotificationRequests_.Add(owner, callback, move(requests));
    }

    void BroadcastEventManager::RemoveRequests(
        AsyncOperationSPtr const & owner,
        set<NamingUri> && names)
    {
        pendingLocationNotificationRequests_.Remove(owner, move(names));
    }

    void BroadcastEventManager::OnBroadcastProcessed(EventArgs const & args)
    {
        BroadcastProcessedEventArgs const & broadcastArgs = dynamic_cast<BroadcastProcessedEventArgs const &>(args);
        
        ServiceTableEntry unusedLocations;
        GenerationNumber unusedGeneration;

        UserServiceCuidMap notifications;

        for (auto & updatedServiceCuids : broadcastArgs.UpdatedCuids)
        {
            auto & name = updatedServiceCuids.first;
            auto & cuids = updatedServiceCuids.second;

            // PSDs are only cached for user services.
            // Broadcasts are used to detect PSD changes and
            // invalidate the cache if needed.
            //
            NamingUri uri;
            if (NamingUri::TryParse(name, uri))
            {
                GatewayPsdCacheEntrySPtr psdCacheEntry;
                if (psdCache_.TryGet(name, psdCacheEntry))
                {
                    for (auto const & cuid : cuids)
                    {
                        if (!psdCacheEntry->Psd->ServiceContains(cuid))
                        {
                            Trace.WriteInfo(
                                TraceComponent,
                                "{0} removed PSD cache on CUID mismatch: name={1} cuid={2}",
                                gatewayTraceId_,
                                name,
                                cuid);

                            psdCache_.TryRemove(name);
                            break;
                        }
                        else if (!resolver_.TryGetCachedEntry(cuid, unusedLocations, unusedGeneration))
                        {
                            Trace.WriteInfo(
                                TraceComponent,
                                "{0} removed PSD cache on RSP cache miss: name={1} cuid={2}",
                                gatewayTraceId_,
                                name,
                                cuid);

                            psdCache_.TryRemove(name);
                            break;
                        }
                    }
                }

                // Notifications only apply to user services
                //
                notifications.insert(pair<NamingUri, vector<ConsistencyUnitId>>(move(uri), move(cuids)));
            }
        }

        // Invalidate cached PSD for a service when any of its partitions is broadcasted with
        // an empty set of endpoints since this typically indicates that the service was
        // either deleted or repartitioned (some partition was removed).
        //
        // An alternative approach for handling repartition would be for FM to include 
        // the service instance and update versions in broadcasted ServiceTableEntries. 
        //
        for (auto & removedServiceCuids : broadcastArgs.RemovedCuids)
        {
            auto & name = removedServiceCuids.first;
            auto & cuids = removedServiceCuids.second;

            NamingUri uri;
            if (NamingUri::TryParse(name, uri))
            {
                psdCache_.TryRemove(name);

                auto it = notifications.find(uri);
                if (it == notifications.end())
                {
                    notifications.insert(pair<NamingUri, vector<ConsistencyUnitId>>(move(uri), move(cuids)));
                }
                else
                {
                    it->second.insert(it->second.end(), cuids.begin(), cuids.end());
                }
            }
        }

        pendingLocationNotificationRequests_.Notify(notifications);
    }
}

