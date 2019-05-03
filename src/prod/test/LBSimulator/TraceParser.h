// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class FM;

    class TraceParser
    {
        DENY_COPY(TraceParser);

    public:
        static std::wstring const PLBConstructTrace;
        static std::wstring const UpdateNode;
        static std::wstring const UpdateApplication;
        static std::wstring const UpdateServiceType;
        static std::wstring const UpdateService;
        static std::wstring const UpdateServicePackage;
        static std::wstring const UpdateFailoverUnitTrace;
        static std::wstring const UpdateLoadOnNode;
        static std::wstring const UpdateLoad;
        static std::wstring const PLBProcessPendingUpdatesTrace;
        static std::wstring const ReplicaFlagsString;
        static std::wstring const FailoverUnitFlagString;
        

        TraceParser(Reliability::LoadBalancingComponent::PLBConfig & plbConfig);

        void Parse(std::wstring const& fileName);

        __declspec (property(get = get_Nodes)) std::vector<Reliability::LoadBalancingComponent::NodeDescription> const& Nodes;
        std::vector<Reliability::LoadBalancingComponent::NodeDescription> const& get_Nodes(){ return nodes_; }

        __declspec (property(get = get_Applications)) std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> const& Applications;
        std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> const& get_Applications(){ return applications_; }

        __declspec (property(get = get_ServiceTypes)) std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> const& ServiceTypes;
        std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> const& get_ServiceTypes(){ return serviceTypes_; }

        __declspec (property(get = get_Services)) std::vector<Reliability::LoadBalancingComponent::ServiceDescription> const& Services;
        std::vector<Reliability::LoadBalancingComponent::ServiceDescription> const& get_Services(){ return services_; }

        __declspec (property(get = get_FailoverUnits)) std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> const& FailoverUnits;
        std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> const& get_FailoverUnits(){ return failoverUnits_; }

        __declspec (property(get = get_LoadAndMoveCosts)) std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> const& LoadAndMoveCosts;
        std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> const& get_LoadAndMoveCosts(){ return loadAndMoveCosts_; }
    private:
        Reliability::LoadBalancingComponent::NodeDescription ParseNodeUpdate(std::wstring const & updateNodeTrace);
        Reliability::LoadBalancingComponent::ApplicationDescription ParseApplicationUpdate(std::wstring const & updateApplicationTrace);
        Reliability::LoadBalancingComponent::ServiceTypeDescription ParseServiceTypeUpdate(std::wstring const & updateServiceTypeTrace);
        Reliability::LoadBalancingComponent::ServiceDescription ParseServiceUpdate(std::wstring const & updateServiceTrace);
        Reliability::LoadBalancingComponent::FailoverUnitDescription ParseFailoverUnitUpdate(std::wstring const & failoverUnitTrace);
        Reliability::LoadBalancingComponent::LoadOrMoveCostDescription ParseLoadOnNodeUpdate(std::wstring const & loadOnNodeUpdateTrace);
        Reliability::LoadBalancingComponent::LoadOrMoveCostDescription ParseLoadUpdate(std::wstring const & loadUpdateTrace);

        bool CheckReplicaFlags(std::wstring const& flags);
        bool CheckFailoverUnitFlags(std::wstring const& flags);

        map<wstring, uint> CreateCapacities(wstring capacityStr);
        vector<Reliability::LoadBalancingComponent::ServiceMetric> CreateMetrics(wstring metrics);

        Reliability::LoadBalancingComponent::PLBConfig & config_;

        std::map<Common::Guid, std::wstring> guidToServiceName_;

        // Parserd Descriptions
        std::vector<Reliability::LoadBalancingComponent::NodeDescription> nodes_;
        std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> applications_;
        std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> serviceTypes_;
        std::vector<Reliability::LoadBalancingComponent::ServiceDescription> services_;
        std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> failoverUnits_;
        std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> loadAndMoveCosts_;
    };
}
