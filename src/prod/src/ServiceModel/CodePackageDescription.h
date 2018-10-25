// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServiceManifest>/<CodePackage>
    struct DigestedCodePackageDescription;
    struct DigestedConfigPackageDescription;
    struct ServiceManifestDescription;
    struct CodePackageDescription : public Serialization::FabricSerializable
    {
    public:
        CodePackageDescription();
        CodePackageDescription(CodePackageDescription const & other);
        CodePackageDescription(CodePackageDescription && other);

        CodePackageDescription const & operator = (CodePackageDescription const & other);
        CodePackageDescription const & operator = (CodePackageDescription && other);

        bool operator == (CodePackageDescription const & other) const;
        bool operator != (CodePackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CODE_PACKAGE_DESCRIPTION const & publicDescription);
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in std::wstring const& serviceManifestName, 
            __in std::wstring const& serviceManifestVersion, 
            __out FABRIC_CODE_PACKAGE_DESCRIPTION & publicDescription) const;

        void clear();
        
        FABRIC_FIELDS_06(Name, Version, IsShared, EntryPoint, SetupEntryPoint, HasSetupEntryPoint);

    public:
        std::wstring Name;
        std::wstring Version;
        bool IsActivator;
        bool IsShared;
        EntryPointDescription EntryPoint;
        ExeEntryPointDescription SetupEntryPoint;
        bool HasSetupEntryPoint;
        EnvironmentVariablesDescription EnvironmentVariables;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct DigestedConfigPackageDescription;
        friend struct ServiceManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        Common::ErrorCode WriteSetupEntryPoint(Common::XmlWriterUPtr const &);
        Common::ErrorCode WriteEntryPoint(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::CodePackageDescription);
