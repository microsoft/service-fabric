// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedServiceTypes> element.
    struct ServicePackageDescription;
    struct DigestedServiceTypesDescription
    {
        DigestedServiceTypesDescription();
        DigestedServiceTypesDescription(DigestedServiceTypesDescription const & other);
        DigestedServiceTypesDescription(DigestedServiceTypesDescription && other);

        DigestedServiceTypesDescription const & operator = (DigestedServiceTypesDescription const & other);
        DigestedServiceTypesDescription const & operator = (DigestedServiceTypesDescription && other);

        bool operator == (DigestedServiceTypesDescription const & other) const;
        bool operator != (DigestedServiceTypesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
    public:
        RolloutVersion RolloutVersionValue;
        std::vector<ServiceTypeDescription> ServiceTypes;    
       
    private:
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseServiceTypes(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatelessServiceType(Common::XmlReaderUPtr const & xmlReader);
        void ParseStatefulServiceType(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
