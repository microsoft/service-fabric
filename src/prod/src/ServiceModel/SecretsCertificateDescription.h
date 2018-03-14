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
    struct SecretsCertificateDescription : public Serialization::FabricSerializable
    {
        SecretsCertificateDescription();
        SecretsCertificateDescription(SecretsCertificateDescription const & other);
        SecretsCertificateDescription(SecretsCertificateDescription && other);

        SecretsCertificateDescription const & operator = (SecretsCertificateDescription const & other);
        SecretsCertificateDescription const & operator = (SecretsCertificateDescription && other);

        bool operator == (SecretsCertificateDescription const & other) const;
        bool operator != (SecretsCertificateDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        
        void clear();

        FABRIC_FIELDS_03(X509FindValue, X509StoreName, X509FindType);

    public:                    
        std::wstring X509StoreName;
        Common::X509FindType::Enum X509FindType;
        std::wstring X509FindValue;  
		std::wstring X509FindValueSecondary;
        std::wstring Name;

    private:
        friend struct DigestedCertificatesDescription;
		friend struct ApplicationManifestDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
