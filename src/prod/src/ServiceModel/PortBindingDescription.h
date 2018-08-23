// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <PortBinding> element.
    struct PortBindingDescription : 
        public Serialization::FabricSerializable
    {
    public:
        PortBindingDescription(); 

        PortBindingDescription(PortBindingDescription const & other) = default;
        PortBindingDescription(PortBindingDescription && other) = default;

        PortBindingDescription & operator = (PortBindingDescription const & other) = default;
        PortBindingDescription & operator = (PortBindingDescription && other) = default;

        bool operator == (PortBindingDescription const & other) const;
        bool operator != (PortBindingDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;


        void clear();

        FABRIC_FIELDS_02(ContainerPort, EndpointResourceRef);

    public:
        ULONG ContainerPort;
        std::wstring EndpointResourceRef;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        friend struct ServicePackageContainerPolicyDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
