// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ResourceOverrides> element.
    struct ServiceManifestImportDescription;
    struct ResourceOverridesDescription : public Serialization::FabricSerializable
    {
        ResourceOverridesDescription();
        ResourceOverridesDescription(ResourceOverridesDescription const & other);
        ResourceOverridesDescription(ResourceOverridesDescription && other);

        ResourceOverridesDescription & operator = (ResourceOverridesDescription const & other);
        ResourceOverridesDescription & operator = (ResourceOverridesDescription && other);

        bool operator == (ResourceOverridesDescription const & other) const;
        bool operator != (ResourceOverridesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_01(Endpoints);

    public:
        std::vector<EndpointOverrideDescription> Endpoints;

    private:
        friend struct ServiceManifestImportDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
        void ParseEndpoints(Common::XmlReaderUPtr const & xmlReader);
        Common::ErrorCode WriteEndpoints(Common::XmlWriterUPtr const & xmlWriter);
        bool NoEndPoints();

    };
}
