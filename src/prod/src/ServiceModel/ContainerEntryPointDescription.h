// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{

    struct EntryPointDescription;
    struct CodePackageDescription;
    struct ContainerEntryPointDescription : public Serialization::FabricSerializable
    {
    public:
        ContainerEntryPointDescription();
        ContainerEntryPointDescription(ContainerEntryPointDescription const & other);
        ContainerEntryPointDescription(ContainerEntryPointDescription && other);

        ContainerEntryPointDescription const & operator = (ContainerEntryPointDescription const & other);
        ContainerEntryPointDescription const & operator = (ContainerEntryPointDescription && other);

        bool operator == (ContainerEntryPointDescription const & other) const;
        bool operator != (ContainerEntryPointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_04(ImageName, Commands, FromSource, EntryPoint);

    public:
        std::wstring ImageName;
        std::wstring Commands;
        std::wstring FromSource;
        std::wstring EntryPoint;
       
    private:
        friend struct CodePackageDescription;
        friend struct EntryPointDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
