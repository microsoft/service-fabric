// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance> element.
    struct Parser;
	struct Serializer;
    struct ApplicationPackageDescription
    {
        ApplicationPackageDescription();
        ApplicationPackageDescription(ApplicationPackageDescription const & other);
        ApplicationPackageDescription(ApplicationPackageDescription && other);

        ApplicationPackageDescription const & operator = (ApplicationPackageDescription const & other);
        ApplicationPackageDescription const & operator = (ApplicationPackageDescription && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        
        void clear();
    
    public:            
        RolloutVersion RolloutVersionValue;
        std::wstring ApplicationId;
        std::wstring ApplicationTypeName;
        std::wstring ApplicationName;
        std::wstring ContentChecksum;

        DigestedEnvironmentDescription DigestedEnvironment;    
        DigestedCertificatesDescription DigestedCertificates;

    private:
        friend struct Parser;
		friend struct Serializer;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
