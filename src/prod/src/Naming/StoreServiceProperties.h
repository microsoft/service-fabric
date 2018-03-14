// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreServiceProperties
    {
        DENY_COPY (StoreServiceProperties)

    public:
        StoreServiceProperties(
            Federation::NodeInstance const & nodeInstance,
            __in INamingStoreServiceRouter & router,
            __in Reliability::ServiceAdminClient & adminClient,
            __in Reliability::ServiceResolver & resolver,
            __in NameRequestInstanceTracker & authorityOwnerRequestTracker,
            __in NameRequestInstanceTracker & nameOwnerRequestTracker,
            __in NamePropertyRequestInstanceTracker & propertyRequestTracker,
            __in Naming::PsdCache & psdCache,
            StoreServiceEventSource const & trace,
            __in StoreServiceHealthMonitor & healthMonitor)
            : nodeInstance_(nodeInstance)
            , router_(router)
            , adminClient_(adminClient)
            , resolver_(resolver)
            , authorityOwnerRequestTracker_(authorityOwnerRequestTracker)
            , nameOwnerRequestTracker_(nameOwnerRequestTracker)
            , propertyRequestTracker_(propertyRequestTracker)
            , perfCounters_()
            , psdCache_(psdCache)
            , trace_(trace)
            , namingServiceCuids_(NamingConfig::GetConfig().PartitionCount)
            , partitionId_(L"")
            , healthMonitor_(healthMonitor)
        {
        }

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & Instance;
        Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }
        
        __declspec(property(get=get_PartitionId, put=set_PartitionId)) std::wstring const & PartitionId;
        std::wstring const & get_PartitionId() const { return partitionId_; }
        void set_PartitionId(std::wstring const & value) { partitionId_ = value; }

        __declspec(property(get=get_Router, put=put_Router)) INamingStoreServiceRouter & Router;
        INamingStoreServiceRouter & get_Router() const { return router_; }

        __declspec(property(get=get_AdminClient)) Reliability::ServiceAdminClient & AdminClient;
        Reliability::ServiceAdminClient & get_AdminClient() const { return adminClient_; }

        __declspec(property(get=get_Resolver)) Reliability::ServiceResolver & Resolver;
        Reliability::ServiceResolver & get_Resolver() const { return resolver_; }

        __declspec(property(get=get_AuthorityOwnerRequestTracker)) NameRequestInstanceTracker & AuthorityOwnerRequestTracker;
        NameRequestInstanceTracker & get_AuthorityOwnerRequestTracker() const { return authorityOwnerRequestTracker_; }

        __declspec(property(get=get_NameOwnerRequestTracker)) NameRequestInstanceTracker & NameOwnerRequestTracker;
        NameRequestInstanceTracker & get_NameOwnerRequestTracker() const { return nameOwnerRequestTracker_; }

        __declspec(property(get=get_PropertyRequestTracker)) NamePropertyRequestInstanceTracker & PropertyRequestTracker;
        NamePropertyRequestInstanceTracker & get_PropertyRequestTracker() const { return propertyRequestTracker_; }

        __declspec(property(get=get_PerfCounters,put=set_PerfCounters)) PerformanceCountersSPtr const & PerfCounters;
        PerformanceCountersSPtr const & get_PerfCounters() const { return perfCounters_; }
        void set_PerfCounters(PerformanceCountersSPtr const & perfCounters) { perfCounters_ = perfCounters; }
        
        __declspec(property(get=get_PsdCache)) Naming::PsdCache & PsdCache;
        Naming::PsdCache & get_PsdCache() const { return psdCache_; }
        
        __declspec(property(get=get_Trace)) Naming::StoreServiceEventSource const & Trace;
        Naming::StoreServiceEventSource const & get_Trace() const { return trace_; }
        
        __declspec(property(get=get_NamingCuidCollection)) NamingServiceCuidCollection const & NamingCuidCollection;
        NamingServiceCuidCollection const & get_NamingCuidCollection() const { return namingServiceCuids_; }

        __declspec(property(get=get_HealthMonitor)) StoreServiceHealthMonitor & HealthMonitor;
        StoreServiceHealthMonitor & get_HealthMonitor() const { return healthMonitor_; }

    private:
        Federation::NodeInstance nodeInstance_;
        std::wstring partitionId_;
        INamingStoreServiceRouter & router_;
        Reliability::ServiceAdminClient & adminClient_;
        Reliability::ServiceResolver & resolver_;
        NameRequestInstanceTracker & authorityOwnerRequestTracker_;
        NameRequestInstanceTracker & nameOwnerRequestTracker_;
        NamePropertyRequestInstanceTracker & propertyRequestTracker_;
        PerformanceCountersSPtr perfCounters_;
        Naming::PsdCache & psdCache_;
        Naming::StoreServiceEventSource const & trace_;
        NamingServiceCuidCollection namingServiceCuids_; 
        StoreServiceHealthMonitor & healthMonitor_;
    };

    typedef std::unique_ptr<StoreServiceProperties> StoreServicePropertiesUPtr;
}
