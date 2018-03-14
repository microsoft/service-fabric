// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EndpointCertificateBinding : public Serialization::FabricSerializable
    {
    public:
        EndpointCertificateBinding();
        EndpointCertificateBinding(
            UINT port,
            bool explicitPort,
            std::wstring const & principalSid,
            std::wstring const & x509CertFindValue,
	        std::wstring const & x509StoreName,
            Common::X509FindType::Enum sslCertificateFindType);

        EndpointCertificateBinding::EndpointCertificateBinding(
            EndpointCertificateBinding const & other);

        EndpointCertificateBinding::EndpointCertificateBinding(
            EndpointCertificateBinding && other);
        
        EndpointCertificateBinding const & operator = (EndpointCertificateBinding const & other);
        EndpointCertificateBinding const & operator = (EndpointCertificateBinding && other);

        bool operator == (EndpointCertificateBinding const & other) const;
        bool operator != (EndpointCertificateBinding const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(Port, IsExplicitPort, PrincipalSid, X509FindValue, X509StoreName, CertFindType);

    public:
        std::wstring PrincipalSid;
        UINT Port;
        bool IsExplicitPort;
        std::wstring X509FindValue;
        std::wstring X509StoreName;
        Common::X509FindType::Enum CertFindType;
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::EndpointCertificateBinding);
