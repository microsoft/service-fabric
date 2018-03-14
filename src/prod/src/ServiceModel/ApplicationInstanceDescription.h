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
    struct ApplicationInstanceDescription
    {
        ApplicationInstanceDescription();
        ApplicationInstanceDescription(ApplicationInstanceDescription const & other);
        ApplicationInstanceDescription(ApplicationInstanceDescription && other);

        ApplicationInstanceDescription const & operator = (ApplicationInstanceDescription const & other);
        ApplicationInstanceDescription const & operator = (ApplicationInstanceDescription && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);
        
        void clear();
    
    public:            
        int Version;
        std::wstring ApplicationId;
        std::wstring ApplicationTypeName;
        std::wstring ApplicationTypeVersion;   
		std::wstring NameUri;
        ApplicationPackageReference ApplicationPackageReference;
        std::vector<ServicePackageReference> ServicePackageReferences;
        std::vector<ApplicationServiceDescription> ServiceTemplates;
        std::vector<DefaultServiceDescription> DefaultServices;        

    private:
        friend struct Parser;
		friend struct Serializer;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseServiceTemplates(Common::XmlReaderUPtr const & xmlReader);
        void ParseDefaultServices(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
