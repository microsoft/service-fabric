// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 
namespace ServiceModel
{
    // <ApplicationManifest> element.
    struct Parser;
	struct ApplicationDefaultServiceDescription;
	struct PrincipalsDescription;
	struct SecretsCertificateDescription;
	struct DiagnosticsDescription;
    struct ApplicationManifestDescription : public Serialization::FabricSerializable
    {
        ApplicationManifestDescription();
        ApplicationManifestDescription(ApplicationManifestDescription const & other);
        ApplicationManifestDescription(ApplicationManifestDescription && other);

        ApplicationManifestDescription const & operator = (ApplicationManifestDescription const & other);
        ApplicationManifestDescription const & operator = (ApplicationManifestDescription && other);

        bool operator == (ApplicationManifestDescription const & other) const;
        bool operator != (ApplicationManifestDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);

        FABRIC_FIELDS_04(Name, Version, ApplicationParameters, ManifestId);

    public:
        std::wstring Name;
        std::wstring Version;
        std::wstring ManifestId;
		std::wstring Description;
		std::map<std::wstring, std::wstring> ApplicationParameters;
		std::vector<ServiceManifestImportDescription> ServiceManifestImports;
		std::vector<ApplicationServiceTemplateDescription> ServiceTemplates;
		std::vector<ApplicationDefaultServiceDescription> DefaultServices;
		PrincipalsDescription Principals;
		ApplicationPoliciesDescription Policies;
		DiagnosticsDescription Diagnostics;
		std::vector<SecretsCertificateDescription> Certificates;
        std::vector<EndpointCertificateDescription> EndpointCertificates;

    private:
        friend struct Parser;
		friend struct Serializer;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseApplicationDefaultParameters(Common::XmlReaderUPtr const & xmlReader);
		void ParseApplicationDefaultParameterElements(Common::XmlReaderUPtr const & xmlReader);
		void ParseDefaultServices(Common::XmlReaderUPtr const & xmlReader);
        void ParseServiceManifestImports(Common::XmlReaderUPtr const & xmlReader);
        void ParsePolicies(Common::XmlReaderUPtr const & xmlReader);
		void ParseServiceTemplates(Common::XmlReaderUPtr const & xmlReader);
		void ParseCertificates(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteParameters(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteDefaultServices(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteServiceTemplates(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteCertificates(Common::XmlWriterUPtr const &);
        void clear();
    };
}
