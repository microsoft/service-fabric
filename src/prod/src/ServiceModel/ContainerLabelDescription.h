// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <PortBinding> element.
    struct ContainerLabelDescription : 
        public Serialization::FabricSerializable
    {
    public:
        ContainerLabelDescription(); 

        ContainerLabelDescription(ContainerLabelDescription const & other) = default;
        ContainerLabelDescription(ContainerLabelDescription && other) = default;

        ContainerLabelDescription & operator = (ContainerLabelDescription const & other) = default;
        ContainerLabelDescription & operator = (ContainerLabelDescription && other) = default;

        bool operator == (ContainerLabelDescription const & other) const;
        bool operator != (ContainerLabelDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;


        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_LABEL_DESCRIPTION & fabricLabelDesc) const;

        FABRIC_FIELDS_02(Name, Value);

    public:
        std::wstring Name;
        std::wstring Value;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ContainerLabelDescription);
