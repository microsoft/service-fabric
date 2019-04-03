// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // <NetworkPolicies> element
    struct ServicePackageDescription;
    struct NetworkPoliciesDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NetworkPoliciesDescription();
        NetworkPoliciesDescription(NetworkPoliciesDescription const & other);
        NetworkPoliciesDescription(NetworkPoliciesDescription && other);

        NetworkPoliciesDescription const & operator = (NetworkPoliciesDescription const & other);
        NetworkPoliciesDescription const & operator = (NetworkPoliciesDescription && other);

        bool operator == (NetworkPoliciesDescription const & other) const;
        bool operator != (NetworkPoliciesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static Common::ErrorCode FromString(std::wstring const & networkPoliciesDescriptionStr, __out NetworkPoliciesDescription & networkPoliciesDescription);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ContainerNetworkPolicies, ContainerNetworkPolicies)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(ContainerNetworkPolicies);

    public:        
        std::vector<ServiceModel::ContainerNetworkPolicyDescription> ContainerNetworkPolicies;

    private:
        friend struct ServicePackageDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void clear();
    };
}