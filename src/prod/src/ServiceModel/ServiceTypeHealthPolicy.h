// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationHealthPolicy;
    struct ServiceTypeHealthPolicy 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceTypeHealthPolicy();
        ServiceTypeHealthPolicy(
            BYTE maxPercentUnhealthyServices,
            BYTE maxPercentUnhealthyPartitionsPerService,
            BYTE maxPercentUnhealthyReplicasPerPartition);
        ServiceTypeHealthPolicy(ServiceTypeHealthPolicy const & other) = default;
        ServiceTypeHealthPolicy & operator = (ServiceTypeHealthPolicy const & other) = default;
        
        ServiceTypeHealthPolicy(ServiceTypeHealthPolicy && other) = default;
        ServiceTypeHealthPolicy & operator = (ServiceTypeHealthPolicy && other) = default;

        bool operator == (ServiceTypeHealthPolicy const & other) const;
        bool operator != (ServiceTypeHealthPolicy const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & serviceTypeHealthPolicyStr, __out ServiceTypeHealthPolicy & serviceTypeHealthPolicy);

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_TYPE_HEALTH_POLICY const & publicServiceTypeHealthPolicy);
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_TYPE_HEALTH_POLICY & publicServiceTypeHealthPolicy) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM const & publicServiceTypeHealthPolicyItem,
            __out std::wstring & serviceTypeName);
        void ToPublicApiMapItem(
            __in Common::ScopedHeap & heap,
            std::wstring const & serviceTypeName,
            __out FABRIC_SERVICE_TYPE_HEALTH_POLICY_MAP_ITEM & publicServiceTypeHealthPolicyItem) const;

        bool TryValidate(__out std::wstring & validationErrorMessage) const;

        void clear();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyServices, MaxPercentUnhealthyServices)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyPartitionsPerService, MaxPercentUnhealthyPartitionsPerService)
            SERIALIZABLE_PROPERTY(Constants::MaxPercentUnhealthyReplicasPerPartition, MaxPercentUnhealthyReplicasPerPartition)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_03(MaxPercentUnhealthyServices, MaxPercentUnhealthyPartitionsPerService, MaxPercentUnhealthyReplicasPerPartition);

    public:
        BYTE MaxPercentUnhealthyServices;
        BYTE MaxPercentUnhealthyPartitionsPerService;
        BYTE MaxPercentUnhealthyReplicasPerPartition;

    private:
        friend struct ApplicationHealthPolicy;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &, bool isDefault, std::wstring const & name = L"");
    };

    typedef std::map<std::wstring, ServiceTypeHealthPolicy> ServiceTypeHealthPolicyMap;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceTypeHealthPolicy);
DEFINE_USER_MAP_UTILITY(std::wstring, ServiceModel::ServiceTypeHealthPolicy);
