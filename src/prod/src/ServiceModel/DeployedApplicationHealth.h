// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedApplicationHealth
        : public EntityHealthBase
    {
        DENY_COPY(DeployedApplicationHealth)
    public:
        DeployedApplicationHealth();

        DeployedApplicationHealth(
            std::wstring const & applicationName,
            std::wstring const & nodeName,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            std::vector<DeployedServicePackageAggregatedHealthState> && deployedServicePackageAggregatedHealthStates,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics);

        DeployedApplicationHealth(DeployedApplicationHealth && other) = default;
        DeployedApplicationHealth & operator = (DeployedApplicationHealth && other) = default;

        virtual ~DeployedApplicationHealth();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }
    
        __declspec(property(get=get_DeployedServicePackagesAggregatedHealthStates)) std::vector<DeployedServicePackageAggregatedHealthState> const & DeployedServicePackagesAggregatedHealthStates;
        std::vector<DeployedServicePackageAggregatedHealthState> const & get_DeployedServicePackagesAggregatedHealthStates() const { return deployedServicePackagesAggregatedHealthStates_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_APPLICATION_HEALTH & publicDeployedApplicationHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_DEPLOYED_APPLICATION_HEALTH const & publicDeployedApplicationHealth);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;
        
        FABRIC_FIELDS_07(applicationName_, nodeName_, events_, aggregatedHealthState_, deployedServicePackagesAggregatedHealthStates_, unhealthyEvaluations_, healthStats_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::DeployedServicePackageAggregatedHealthState, deployedServicePackagesAggregatedHealthStates_)
        END_JSON_SERIALIZABLE_PROPERTIES()
        
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deployedServicePackagesAggregatedHealthStates_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring nodeName_;
        std::vector<DeployedServicePackageAggregatedHealthState> deployedServicePackagesAggregatedHealthStates_;
    };    
}
