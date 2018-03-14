// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <NetworkConfig> element.
    struct NetworkConfigDescription : 
        public Serialization::FabricSerializable
    {
    public:
        NetworkConfigDescription();

        NetworkConfigDescription(NetworkConfigDescription const & other) = default;
        NetworkConfigDescription(NetworkConfigDescription && other) = default;

        NetworkConfigDescription & operator = (NetworkConfigDescription const & other) = default;
        NetworkConfigDescription & operator = (NetworkConfigDescription && other) = default;

        bool operator == (NetworkConfigDescription const & other) const;
        bool operator != (NetworkConfigDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;


        void clear();

        FABRIC_FIELDS_01(Type);

    public:
        NetworkType::Enum Type;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::NetworkConfigDescription);
