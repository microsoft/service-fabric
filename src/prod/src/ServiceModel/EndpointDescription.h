// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Endpoint> element.
    struct DigestedEndpointDescription;
    struct ResourcesDescription;
    struct EndpointDescription : public Serialization::FabricSerializable
    {
        EndpointDescription();
        EndpointDescription(EndpointDescription const & other) = default;
        EndpointDescription(EndpointDescription && other) = default;

        EndpointDescription & operator = (EndpointDescription const & other) = default;
        EndpointDescription & operator = (EndpointDescription && other) = default;

        bool operator == (EndpointDescription const & other) const;
        bool operator != (EndpointDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static bool WriteToFile(std::wstring const& fileName, std::vector<EndpointDescription> const& endpointDescriptions);
        static bool ReadFromFile(std::wstring const& fileName, std::vector<EndpointDescription> & endpointDescriptions);

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_ENDPOINT_RESOURCE_DESCRIPTION & publicDescription) const;

        static void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in std::vector<EndpointDescription> const & endpoints, 
            __out  FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST & publicEndpoints);

        void clear();

        FABRIC_FIELDS_09(Name, Protocol, Type, CertificateRef, Port, UriScheme, PathSuffix, CodePackageRef, IpAddressOrFqdn);

    public:
        std::wstring Name;
        ProtocolType::Enum Protocol;
        EndpointType::Enum Type;
        std::wstring CertificateRef;
        ULONG Port;
        bool ExplicitPortSpecified;
        std::wstring UriScheme;
        std::wstring PathSuffix;
        std::wstring CodePackageRef;
        std::wstring IpAddressOrFqdn;

    private:
        friend struct DigestedEndpointDescription;
        friend struct ResourcesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::EndpointDescription);
