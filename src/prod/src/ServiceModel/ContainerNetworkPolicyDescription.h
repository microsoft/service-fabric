// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    /* <ContainerNetworkPolicy> element
       Example:
       <ContainerNetworkPolicy NetworkRef="Network1">
          <EndpointBinding EndpointRef="Endpoint1" />
       </ContainerNetworkPolicy>
    */
    struct NetworkPoliciesDescription;
    struct ContainerNetworkPolicyDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ContainerNetworkPolicyDescription();
        ContainerNetworkPolicyDescription(ContainerNetworkPolicyDescription const & other);
        ContainerNetworkPolicyDescription(ContainerNetworkPolicyDescription && other);

        ContainerNetworkPolicyDescription const & operator = (ContainerNetworkPolicyDescription const & other);
        ContainerNetworkPolicyDescription const & operator = (ContainerNetworkPolicyDescription && other);

        bool operator == (ContainerNetworkPolicyDescription const & other) const;
        bool operator != (ContainerNetworkPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static Common::ErrorCode FromString(std::wstring const & containerNetworkPolicyDescriptionStr, __out ContainerNetworkPolicyDescription & containerNetworkPolicyDescription);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NetworkRef, NetworkRef)
            SERIALIZABLE_PROPERTY(Constants::EndpointBindings, EndpointBindings)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_02(NetworkRef, EndpointBindings);

    public:
        std::wstring NetworkRef;
        std::vector<ServiceModel::ContainerNetworkPolicyEndpointBindingDescription> EndpointBindings;

    private:
        friend struct NetworkPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void clear();
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ContainerNetworkPolicyDescription)