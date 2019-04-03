// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance> element.
    struct SecretsCertificateDescription
    {
        SecretsCertificateDescription();
        SecretsCertificateDescription(SecretsCertificateDescription const & other);
        SecretsCertificateDescription(SecretsCertificateDescription && other);

        SecretsCertificateDescription const & operator = (SecretsCertificateDescription const & other);
        SecretsCertificateDescription const & operator = (SecretsCertificateDescription && other);

        bool operator == (SecretsCertificateDescription const & other) const;
        bool operator != (SecretsCertificateDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        void ReadFromXml(Common::XmlReaderUPtr const & xmlReader);
        
        void clear();
    
    public:                    
        std::wstring X509StoreName;
        Common::X509FindType::Enum X509FindType;
        std::wstring X509FindValue;                
    };
}
