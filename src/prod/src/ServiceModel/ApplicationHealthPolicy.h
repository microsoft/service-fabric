// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationHealthPolicy;
    using ApplicationHealthPolicySPtr = std::shared_ptr<ApplicationHealthPolicy>;

    struct ApplicationManifestDescription;
    struct ApplicationPoliciesDescription;
    struct ApplicationHealthPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationHealthPolicy();
        ApplicationHealthPolicy(
            bool considerWarningAsError,
            BYTE maxPercentUnhealthyDeployedApplications,
            ServiceTypeHealthPolicy const & defaultServiceTypeHealthPolicy,
            ServiceTypeHealthPolicyMap const & serviceTypeHealthPolicyMap);

        ApplicationHealthPolicy(ApplicationHealthPolicy const & other) = default;
        ApplicationHealthPolicy & operator = (ApplicationHealthPolicy const & other) = default;
        
        ApplicationHealthPolicy(ApplicationHealthPolicy && other) = default;
        ApplicationHealthPolicy & operator = (ApplicationHealthPolicy && other) = default;

        static Common::Global<ApplicationHealthPolicy> Default;

        bool operator == (ApplicationHealthPolicy const & other) const;
        bool operator != (ApplicationHealthPolicy const & other) const;

        ServiceTypeHealthPolicy const & GetServiceTypeHealthPolicy(std::wstring const & serviceTypeName) const;
        
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & applicationHealthPolicyStr, __out ApplicationHealthPolicy & applicationHealthPolicy);

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_POLICY const & publicAppHealthPolicy);
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_HEALTH_POLICY & publicAppHealthPolicy) const;

        void ToPublicApiMapItem(
            __in Common::ScopedHeap & heap,
            std::wstring const & applicationName,
            __out FABRIC_APPLICATION_HEALTH_POLICY_MAP_ITEM & publicApplicationHealthPolicyItem) const;

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        void clear();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ConsiderWarningAsError, ConsiderWarningAsError)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyDeployedApplications, MaxPercentUnhealthyDeployedApplications)
            SERIALIZABLE_PROPERTY(Constants::DefaultServiceTypeHealthPolicy, DefaultServiceTypeHealthPolicy)
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeHealthPolicyMap, ServiceTypeHealthPolicies)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(ConsiderWarningAsError, MaxPercentUnhealthyDeployedApplications, DefaultServiceTypeHealthPolicy, ServiceTypeHealthPolicies);

    private:
        friend struct ApplicationManifestDescription;
		friend struct ApplicationPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ReadDefaultServiceTypeHealthPolicy(Common::XmlReaderUPtr const & xmlReader);
        void ReadServiceTypeHealthPolicyMap(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
        
    public:
        bool ConsiderWarningAsError;
        BYTE MaxPercentUnhealthyDeployedApplications;
        ServiceTypeHealthPolicy DefaultServiceTypeHealthPolicy;
        ServiceTypeHealthPolicyMap ServiceTypeHealthPolicies;
    };
}
     
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ApplicationHealthPolicy);
DEFINE_USER_MAP_UTILITY(std::wstring, ServiceModel::ApplicationHealthPolicy);

