// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Endpoint> element for ResourceOverrides in ServiceManifestImport section of ApplicationManifest.
    // This will override the Endpoints specified in ServiceManifest Resource section.
    struct DigestedEndpointOverrideDescription;
    struct ResourceOverridesDescription;
    struct EndpointOverrideDescription : public Serialization::FabricSerializable
    {
        EndpointOverrideDescription();
        EndpointOverrideDescription(EndpointOverrideDescription const & other);
        EndpointOverrideDescription(EndpointOverrideDescription && other);

        EndpointOverrideDescription & operator = (EndpointOverrideDescription const & other);
        EndpointOverrideDescription & operator = (EndpointOverrideDescription && other);

        bool operator == (EndpointOverrideDescription const & other) const;
        bool operator != (EndpointOverrideDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_06(Name, Protocol, Type, Port, UriScheme, PathSuffix);

    public:
        std::wstring Name;
        std::wstring Protocol;
        std::wstring Type;
        std::wstring Port;
        bool ExplicitPortSpecified;
        std::wstring UriScheme;
        std::wstring PathSuffix;

    private:
        friend struct DigestedEndpointOverrideDescription;
        friend struct ResourceOverridesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::EndpointOverrideDescription);
