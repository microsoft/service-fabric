// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct CodePackageDescription;
    struct EntryPointDescription : public Serialization::FabricSerializable
    {
    public:
        EntryPointDescription();
        EntryPointDescription(EntryPointDescription const & other);
        EntryPointDescription(EntryPointDescription && other);

        EntryPointDescription const & operator = (EntryPointDescription const & other);
        EntryPointDescription const & operator = (EntryPointDescription && other);

        bool operator == (EntryPointDescription const & other) const;
        bool operator != (EntryPointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION const & publicDescription);
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION & publicDescription) const;

        void clear();

        __declspec(property(get=get_IsolationPolicy)) CodePackageIsolationPolicyType::Enum IsolationPolicy;
        CodePackageIsolationPolicyType::Enum get_IsolationPolicy() const;

        FABRIC_FIELDS_03(EntryPointType, DllHostEntryPoint, ExeEntryPoint);

    public:
        EntryPointType::Enum EntryPointType;

        DllHostEntryPointDescription DllHostEntryPoint;
        ExeEntryPointDescription ExeEntryPoint;
        ContainerEntryPointDescription ContainerEntryPoint;

    private:
        friend struct CodePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
} 
