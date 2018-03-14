// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class UDDeployedApplicationsHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(UDDeployedApplicationsHealthEvaluation)

    public:
        UDDeployedApplicationsHealthEvaluation();

        UDDeployedApplicationsHealthEvaluation(
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::wstring const & upgradeDomainName,
            HealthEvaluationList && unhealthyEvaluations,
            ULONG totalCount,
            BYTE maxPercentUnhealthyDeployedApplications);

        UDDeployedApplicationsHealthEvaluation(UDDeployedApplicationsHealthEvaluation && other) = default;
        UDDeployedApplicationsHealthEvaluation & operator = (UDDeployedApplicationsHealthEvaluation && other) = default;

        __declspec(property(get=get_MaxPercentUnhealthyDeployedApplications)) BYTE MaxPercentUnhealthyDeployedApplications;
        BYTE get_MaxPercentUnhealthyDeployedApplications() const { return maxPercentUnhealthyDeployedApplications_; }

         __declspec(property(get=get_UpgradeDomainName)) std::wstring const & UpgradeDomainName;
        std::wstring const & get_UpgradeDomainName() const { return upgradeDomainName_; }
        
        __declspec(property(get=get_TotalCount)) ULONG TotalCount;
        ULONG get_TotalCount() const { return totalCount_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_07(kind_, description_, upgradeDomainName_, unhealthyEvaluations_, maxPercentUnhealthyDeployedApplications_, totalCount_, aggregatedHealthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::UpgradeDomainName, upgradeDomainName_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyDeployedApplications, maxPercentUnhealthyDeployedApplications_)
            SERIALIZABLE_PROPERTY(Constants::TotalCount, totalCount_); 
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(upgradeDomainName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring upgradeDomainName_;
        BYTE maxPercentUnhealthyDeployedApplications_;
        ULONG totalCount_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( UDDeployedApplicationsHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS )
}
