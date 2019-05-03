// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationHealthAggregationPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationHealthAggregationPolicy();
        ApplicationHealthAggregationPolicy(
            ApplicationHealthPolicy && applicationPolicy,
            BYTE maxPercentDeployedApplicationsUnhealthyPerUpgradeDomain);

        virtual ~ApplicationHealthAggregationPolicy();

        __declspec(property(get=get_ApplicationPolicy)) ApplicationHealthPolicy const & ApplicationPolicy;
        inline ApplicationHealthPolicy const & get_ApplicationPolicy() const { return applicationPolicy_; };

        __declspec(property(get=get_MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain)) BYTE MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain;
        inline BYTE get_MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain() const { return maxPercentDeployedApplicationsUnhealthyPerUpgradeDomain_; };

        bool operator == (ApplicationHealthAggregationPolicy const & other) const;
        bool operator != (ApplicationHealthAggregationPolicy const & other) const;

        ApplicationHealthAggregationPolicy & operator = (ApplicationHealthAggregationPolicy && other);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_AGGREGATION_POLICY const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_APPLICATION_HEALTH_AGGREGATION_POLICY &) const;

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_AGGREGATION_POLICY_ITEM const &, __out std::wstring & applicationName);
        void ToPublicApi(__in Common::ScopedHeap &, std::wstring const & applicationName, __out FABRIC_APPLICATION_HEALTH_AGGREGATION_POLICY_ITEM &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const &, __out ApplicationHealthAggregationPolicy &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthPolicy, applicationPolicy_)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentDeployedApplicationsUnhealthyPerUpgradeDomain, maxPercentDeployedApplicationsUnhealthyPerUpgradeDomain_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_02(applicationPolicy_, maxPercentDeployedApplicationsUnhealthyPerUpgradeDomain_);

    private:
        ApplicationHealthPolicy applicationPolicy_;
        BYTE maxPercentDeployedApplicationsUnhealthyPerUpgradeDomain_;
    };

    typedef std::map<std::wstring, ApplicationHealthAggregationPolicy> ApplicationHealthAggregationPolicyMap;
    DEFINE_USER_ARRAY_UTILITY(ApplicationHealthAggregationPolicy);
    DEFINE_USER_MAP_UTILITY(std::wstring, ApplicationHealthAggregationPolicy);
}
