// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Resources> element.
    struct ServiceManifestDescription;
    struct ResourcesDescription : public Serialization::FabricSerializable
    {
        ResourcesDescription();
        ResourcesDescription(ResourcesDescription const & other);
        ResourcesDescription(ResourcesDescription && other);

        ResourcesDescription const & operator = (ResourcesDescription const & other);
        ResourcesDescription const & operator = (ResourcesDescription && other);

        bool operator == (ResourcesDescription const & other) const;
        bool operator != (ResourcesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_01(Endpoints);

    public:
        std::vector<EndpointDescription> Endpoints;
        // TODO: add certificates

    private:
        friend struct ServiceManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void ParseCertificates(Common::XmlReaderUPtr const & xmlReader);
        void ParseEndpoints(Common::XmlReaderUPtr const & xmlReader);
		Common::ErrorCode WriteEndpoints(Common::XmlWriterUPtr const & xmlWriter);
		bool noEndPoints();



		//TODO: When implementing certificates, also implement these methods correctly
		bool noCertificates();
		Common::ErrorCode WriteCertificates(Common::XmlWriterUPtr const &);
    };
}
