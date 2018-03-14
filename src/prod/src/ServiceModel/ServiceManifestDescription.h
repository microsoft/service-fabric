// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServiceManifest> element.
    struct Parser;
	struct Serializer;
    struct ServiceManifestDescription : public Serialization::FabricSerializable
    {
        ServiceManifestDescription();
        ServiceManifestDescription(ServiceManifestDescription const & other);
        ServiceManifestDescription(ServiceManifestDescription && other);

        ServiceManifestDescription const & operator = (ServiceManifestDescription const & other);
        ServiceManifestDescription const & operator = (ServiceManifestDescription && other);

        bool operator == (ServiceManifestDescription const & other) const;
        bool operator != (ServiceManifestDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);

        void clear();

        FABRIC_FIELDS_08(Name, Version, ServiceTypeNames, ServiceGroupTypes, CodePackages, ConfigPackages, DataPackages, Resources);

    public:
        std::wstring Name;
        std::wstring Version;
        std::wstring ManifestId;
        std::wstring Description;
        std::vector<ServiceTypeDescription> ServiceTypeNames;
        std::vector<ServiceGroupTypeDescription> ServiceGroupTypes;
        std::vector<CodePackageDescription> CodePackages;
        std::vector<ConfigPackageDescription> ConfigPackages;
        std::vector<DataPackageDescription> DataPackages;
        ResourcesDescription Resources;
        ServiceDiagnosticsDescription Diagnostics;

    private:
        friend struct Parser;
        friend struct Serializer;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseServiceTypes(Common::XmlReaderUPtr const & xmlReader);
        void ParseServiceGroupTypes(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatelessServiceType(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatefulServiceType(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatelessServiceGroupType(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatefulServiceGroupType(Common::XmlReaderUPtr const & xmlReader);
        void ParseCodePackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseConfigPackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseDataPackages(Common::XmlReaderUPtr const & xmlReader);
        void ParseResources(Common::XmlReaderUPtr const & xmlReader);
        void ParseDiagnostics(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteServiceTypes(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteServiceGroupTypes(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteCodePackages(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteConfigPackages(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteDataPackages(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteResources(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteDiagnostics(Common::XmlWriterUPtr const & xmlWriter);
    };
}
