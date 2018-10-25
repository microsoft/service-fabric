// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ServiceMetric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceDescription
        {
            DENY_COPY_ASSIGNMENT(ServiceDescription);

        public:
            static Common::GlobalWString const FormatHeader;

            ServiceDescription(
                std::wstring && serviceName, 
                std::wstring && servicTypeName,
                std::wstring && applicationName,
                bool isStateful,
                std::wstring && placementConstraints,
                std::wstring && affinitizedService,
                bool alignedAffinity,
                std::vector<ServiceMetric> && metrics,
                uint defaultMoveCost,
                bool onEveryNode,
                int partitionCount = 0,
                int targetReplicaSetSize = 0,
                bool hasPersistedState = true,
                ServiceModel::ServicePackageIdentifier && servicePackageIdentifier = ServiceModel::ServicePackageIdentifier(),
                ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode = ServiceModel::ServicePackageActivationMode::SharedProcess,
                uint64 serviceInstance = 0,
                vector<Reliability::ServiceScalingPolicyDescription> && scalingPolicies = vector<Reliability::ServiceScalingPolicyDescription>());

            ServiceDescription(ServiceDescription const & other);

            ServiceDescription(ServiceDescription && other);

            ServiceDescription & operator = (ServiceDescription && other);

            bool operator == (ServiceDescription const& other) const;
            bool operator != (ServiceDescription const& other) const;

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return serviceName_; }

            __declspec (property(get=get_ServiceTypeName)) std::wstring const& ServiceTypeName;
            std::wstring const& get_ServiceTypeName() const { return serviceTypeName_; }

            __declspec (property(get = get_ApplicationName)) std::wstring const& ApplicationName;
            std::wstring const& get_ApplicationName() const { return applicationName_; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return isStateful_; }

            __declspec (property(get = get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const { return hasPersistedState_; }

            __declspec (property(get=get_PlacementConstraints)) std::wstring const& PlacementConstraints;
            std::wstring const& get_PlacementConstraints() const { return placementConstraints_; }

            __declspec (property(get=get_AffinitizedService)) std::wstring const& AffinitizedService;
            std::wstring const& get_AffinitizedService() const { return affinitizedService_; }

            __declspec (property(get=get_AlignedAffinity)) bool AlignedAffinity;
            bool get_AlignedAffinity() const { return alignedAffinity_; }

            __declspec (property(get=get_Metrics)) std::vector<ServiceMetric> const& Metrics;
            std::vector<ServiceMetric> const& get_Metrics() const { return metrics_; }

            __declspec (property(get=get_DefaultMoveCost)) uint DefaultMoveCost;
            uint get_DefaultMoveCost() const { return defaultMoveCost_; }

            __declspec (property(get=get_OnEveryNode)) bool OnEveryNode;
            bool get_OnEveryNode() const { return onEveryNode_; }

            __declspec (property(get=get_PartitionCount)) int PartitionCount;
            int get_PartitionCount() const { return partitionCount_; }

            __declspec (property(get=get_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

            __declspec(property(get = get_ServicePackageIdentifier)) ServiceModel::ServicePackageIdentifier const& ServicePackageIdentifier;
            ServiceModel::ServicePackageIdentifier const& get_ServicePackageIdentifier() const { return servicePackageIdentifier_; }

            __declspec (property(get = get_ServiceId, put = set_ServiceId)) uint64 ServiceId;
            uint64 get_ServiceId() const { return serviceId_; }
            void set_ServiceId(uint64 id) { serviceId_ = id; }

            __declspec (property(get = get_ApplicationId, put = set_ApplicationId)) uint64 ApplicationId;
            uint64 get_ApplicationId() const { return applicationId_; }
            void set_ApplicationId(uint64 id) { applicationId_ = id; }

            __declspec (property(get = get_ServicePackageId, put = set_ServicePackageId)) uint64 ServicePackageId;
            uint64 get_ServicePackageId() const { return servicePackageId_; }
            void set_ServicePackageId(uint64 id) { servicePackageId_ = id; }

            __declspec (property(get = get_ServicePackageActivationMode)) ServiceModel::ServicePackageActivationMode::Enum ServicePackageActivationMode;
            ServiceModel::ServicePackageActivationMode::Enum get_ServicePackageActivationMode() const { return servicePackageActivationMode_; }

            __declspec(property(get = get_IsAutoScalingDefined)) bool IsAutoScalingDefined;
            bool get_IsAutoScalingDefined() const { return scalingPolicies_.size() > 0; }

            // Currently we support only one auto scaling policy per service.
            __declspec(property(get = get_FirstAutoScalingPolicy)) Reliability::ServiceScalingPolicyDescription const& AutoScalingPolicy;
            Reliability::ServiceScalingPolicyDescription get_FirstAutoScalingPolicy() const;

            __declspec (property(get = get_ServiceScalingPolicies)) vector<Reliability::ServiceScalingPolicyDescription> const & ScalingPolicies;
            vector<Reliability::ServiceScalingPolicyDescription> const & get_ServiceScalingPolicies() const { return scalingPolicies_; }

            __declspec (property(get = get_ServiceInstance, put = set_ServiceInstance)) uint64 ServiceInstance;
            uint64 get_ServiceInstance() const { return serviceInstance_; }
            void set_ServiceInstance(int64 instance) { serviceInstance_ = instance; }

            bool AddRGMetric(wstring name, double weight, uint primaryLoad, uint secondaryLoad);

            bool HasRGMetric() const;

            bool AddDefaultMetrics();

            void AlignSinletonReplicaDefLoads();

            void UpdateRGMetrics(std::map<std::wstring, uint> const & resources);

            void ClearAffinitizedService();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring serviceName_;
            std::wstring serviceTypeName_;
            std::wstring applicationName_;
            bool isStateful_;
            std::wstring placementConstraints_;
            std::wstring affinitizedService_;
            bool alignedAffinity_;
            std::vector<ServiceMetric> metrics_;
            uint defaultMoveCost_;
            bool onEveryNode_;
            int partitionCount_;
            int targetReplicaSetSize_;
            bool hasPersistedState_;
            uint64 serviceId_;
            uint64 applicationId_;
            uint64 servicePackageId_;
            ServiceModel::ServicePackageIdentifier servicePackageIdentifier_;
            ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode_;
            uint64 serviceInstance_;
            vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies_;
        };
    }
}
