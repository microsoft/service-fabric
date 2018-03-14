// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance> element.
    struct DigestedCertificatesDescription;
	struct ApplicationManifestDescription;
    struct EndpointCertificateDescription : public Serialization::FabricSerializable
    {
        EndpointCertificateDescription();
        EndpointCertificateDescription(EndpointCertificateDescription const & other);
        EndpointCertificateDescription(EndpointCertificateDescription && other);

        EndpointCertificateDescription const & operator = (EndpointCertificateDescription const & other);
        EndpointCertificateDescription const & operator = (EndpointCertificateDescription && other);

        bool operator == (EndpointCertificateDescription const & other) const;
        bool operator != (EndpointCertificateDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        
        void clear();

        FABRIC_FIELDS_03(X509FindValue, X509StoreName, X509FindType);

    public:                    
        std::wstring X509StoreName;
        Common::X509FindType::Enum X509FindType;
        std::wstring X509FindValue;  
        std::wstring Name;

    private:
        friend struct DigestedCertificatesDescription;
		friend struct ApplicationManifestDescription;
        friend struct DigestedResourcesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
