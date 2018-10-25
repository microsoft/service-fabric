// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationHealth
        : public EntityHealthBase
    {
        DENY_COPY(ApplicationHealth)
    public:
        ApplicationHealth();

        ApplicationHealth(
            std::wstring const & applicationName,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            std::vector<ServiceAggregatedHealthState> && servicesAggregatedHealthStates,
            std::vector<DeployedApplicationAggregatedHealthState> && deployedApplicationsAggregatedHealthStates,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics);

        ApplicationHealth(ApplicationHealth && other) = default;
        ApplicationHealth & operator = (ApplicationHealth && other) = default;

        virtual ~ApplicationHealth();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ServicesAggregatedHealthStates)) std::vector<ServiceAggregatedHealthState> const & ServicesAggregatedHealthStates;
        std::vector<ServiceAggregatedHealthState> const & get_ServicesAggregatedHealthStates() const { return servicesAggregatedHealthStates_; }

        __declspec(property(get=get_DeployedApplicationsAggregatedHealthStates)) std::vector<DeployedApplicationAggregatedHealthState> const & DeployedApplicationsAggregatedHealthStates;
        std::vector<DeployedApplicationAggregatedHealthState> const & get_DeployedApplicationsAggregatedHealthStates() const { return deployedApplicationsAggregatedHealthStates_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_HEALTH & publicApplicationHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_APPLICATION_HEALTH const & publicApplicationHealth);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;
                
        FABRIC_FIELDS_07(applicationName_, events_, aggregatedHealthState_, servicesAggregatedHealthStates_, deployedApplicationsAggregatedHealthStates_, unhealthyEvaluations_, healthStats_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceAggregatedHealthStates, servicesAggregatedHealthStates_)
            SERIALIZABLE_PROPERTY(Constants::DeployedApplicationsAggregatedHealthStates, deployedApplicationsAggregatedHealthStates_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(servicesAggregatedHealthStates_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deployedApplicationsAggregatedHealthStates_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::vector<ServiceAggregatedHealthState> servicesAggregatedHealthStates_;
        std::vector<DeployedApplicationAggregatedHealthState> deployedApplicationsAggregatedHealthStates_;
    };
}
