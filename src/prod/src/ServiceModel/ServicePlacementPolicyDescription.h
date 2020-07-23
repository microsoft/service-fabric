// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    struct ApplicationServiceDescription;
    struct ServiceTypeDescription;
	struct ApplicationServiceTemplateDescription;
    struct ServicePlacementPolicyDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServicePlacementPolicyDescription();
        ServicePlacementPolicyDescription(std::wstring const& domainName, FABRIC_PLACEMENT_POLICY_TYPE type);
        ServicePlacementPolicyDescription(ServicePlacementPolicyDescription const& other);
        ServicePlacementPolicyDescription(ServicePlacementPolicyDescription && other);

        ServicePlacementPolicyDescription & operator = (ServicePlacementPolicyDescription const& other);
        ServicePlacementPolicyDescription & operator = (ServicePlacementPolicyDescription && other);

        bool operator == (ServicePlacementPolicyDescription const & other) const;
        bool operator != (ServicePlacementPolicyDescription const & other) const;

        Common::ErrorCode Equals(ServicePlacementPolicyDescription const & other) const;

        __declspec (property(get=get_DomainName)) std::wstring const& DomainName;
        std::wstring const& get_DomainName() const { return domainName_; }

        __declspec (property(get=get_Type)) FABRIC_PLACEMENT_POLICY_TYPE Type;
        FABRIC_PLACEMENT_POLICY_TYPE get_Type() const { return type_; }

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        void ToPlacementConstraints(__out std::wstring & placementConstraints) const;
        FABRIC_PLACEMENT_POLICY_TYPE GetType(std::wstring const & val);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDescription) const;
        std::wstring FabricPlacementPolicyTypeToString() const;

        void clear();
        std::wstring ToString() const;

        FABRIC_FIELDS_02(domainName_, type_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::DomainName, domainName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Type, type_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(domainName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    public:

        std::wstring domainName_;
        FABRIC_PLACEMENT_POLICY_TYPE type_;

    private:
        friend struct ApplicationServiceDescription;
        friend struct ServiceTypeDescription;
		friend struct ApplicationServiceTemplateDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServicePlacementPolicyDescription);
