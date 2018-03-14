// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ContainerVolumeDescription> element.
    struct ContainerVolumeDescription : 
        public Serialization::FabricSerializable
    {
    public:
        ContainerVolumeDescription();

        ContainerVolumeDescription(ContainerVolumeDescription const & other) = default;
        ContainerVolumeDescription(ContainerVolumeDescription && other) = default;

        ContainerVolumeDescription & operator = (ContainerVolumeDescription const & other) = default;
        ContainerVolumeDescription & operator = (ContainerVolumeDescription && other) = default;

        bool operator == (ContainerVolumeDescription const & other) const;
        bool operator != (ContainerVolumeDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_VOLUME_DESCRIPTION & fabricDriverOptionDesc) const;

        FABRIC_FIELDS_05(Source, Destination, Driver, IsReadOnly, DriverOpts);

    public:
        std::wstring Source;
        std::wstring Destination;
        std::wstring Driver;
        bool IsReadOnly;
        std::vector<DriverOptionDescription> DriverOpts;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ContainerVolumeDescription);
