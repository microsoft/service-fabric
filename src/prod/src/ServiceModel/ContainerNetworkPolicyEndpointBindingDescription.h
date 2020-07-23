// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // <EndpointBinding> element in <ContainerNetworkPolicy> element.
    struct ContainerNetworkPolicyDescription;
    struct ContainerNetworkPolicyEndpointBindingDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        ContainerNetworkPolicyEndpointBindingDescription();
        ContainerNetworkPolicyEndpointBindingDescription(ContainerNetworkPolicyEndpointBindingDescription const & other);
        ContainerNetworkPolicyEndpointBindingDescription(ContainerNetworkPolicyEndpointBindingDescription && other);

        ContainerNetworkPolicyEndpointBindingDescription const & operator = (ContainerNetworkPolicyEndpointBindingDescription const & other);
        ContainerNetworkPolicyEndpointBindingDescription const & operator = (ContainerNetworkPolicyEndpointBindingDescription && other);

        bool operator == (ContainerNetworkPolicyEndpointBindingDescription const & other) const;
        bool operator != (ContainerNetworkPolicyEndpointBindingDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static Common::ErrorCode FromString(std::wstring const & containerNetworkPolicyEndpointBindingDescriptionStr, __out ContainerNetworkPolicyEndpointBindingDescription & containerNetworkPolicyEndpointBindingDescription);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::EndpointRef, EndpointRef)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(EndpointRef);

    public:
        std::wstring EndpointRef;

    private:
        friend struct ContainerNetworkPolicyDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void clear();
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ContainerNetworkPolicyEndpointBindingDescription)
