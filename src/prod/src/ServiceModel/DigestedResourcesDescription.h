// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedResources> element.
    struct ServicePackageDescription;
    struct DigestedResourcesDescription
    {
        DigestedResourcesDescription();
        DigestedResourcesDescription(DigestedResourcesDescription const & other);
        DigestedResourcesDescription(DigestedResourcesDescription && other);

        DigestedResourcesDescription const & operator = (DigestedResourcesDescription const & other);
        DigestedResourcesDescription const & operator = (DigestedResourcesDescription && other);

        bool operator == (DigestedResourcesDescription const & other) const;
        bool operator != (DigestedResourcesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        RolloutVersion RolloutVersionValue;
        std::map<std::wstring, DigestedEndpointDescription> DigestedEndpoints;
        std::map<std::wstring, EndpointCertificateDescription> DigestedCertificates;

    private:
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void DigestedResourcesDescription::ParseDigestedEndpoints(Common::XmlReaderUPtr const & xmlReader);
        void DigestedResourcesDescription::ParseDigestedCertificates(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteDigestedEndPoints(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteDigestedCertificates(Common::XmlWriterUPtr const &);
    };
}
