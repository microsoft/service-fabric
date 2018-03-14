// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedDataPackage> element.
    struct ApplicationPackageDescription;
    struct DigestedCertificatesDescription
    {
        DigestedCertificatesDescription();
        DigestedCertificatesDescription(DigestedCertificatesDescription const & other);
        DigestedCertificatesDescription(DigestedCertificatesDescription && other);

        DigestedCertificatesDescription const & operator = (DigestedCertificatesDescription const & other);
        DigestedCertificatesDescription const & operator = (DigestedCertificatesDescription && other);

        bool operator == (DigestedCertificatesDescription const & other) const;
        bool operator != (DigestedCertificatesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        void ParseCertificateDescription(Common::XmlReaderUPtr const & xmlReader);

        RolloutVersion RolloutVersionValue;
        std::vector<SecretsCertificateDescription> SecretsCertificate;
        std::vector<EndpointCertificateDescription> EndpointCertificates;
    private:
        friend struct ApplicationPackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ReadSecretCertificates(Common::XmlReaderUPtr const &);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

    };
}
