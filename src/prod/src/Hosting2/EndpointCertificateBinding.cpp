// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Common::X509FindType;
using namespace ServiceModel;
using namespace Hosting2;

EndpointCertificateBinding::EndpointCertificateBinding() 
    : Port(0),
    IsExplicitPort(false),
    PrincipalSid(),
    X509FindValue(),
    X509StoreName(),
    CertFindType(FindByThumbprint)
{
}

EndpointCertificateBinding::EndpointCertificateBinding(
    UINT port,
    bool isExplicitPort, 
    wstring const & principalSid,
    wstring const & x509FindValue,
    wstring const & x509StoreName,
    X509FindType::Enum x509FindType) 
    : Port(port),
    IsExplicitPort(isExplicitPort),
    PrincipalSid(principalSid),
    X509FindValue(x509FindValue),
    X509StoreName(x509StoreName),
    CertFindType(x509FindType)
{
}

EndpointCertificateBinding::EndpointCertificateBinding(
    EndpointCertificateBinding const & other) 
    : Port(other.Port),
    IsExplicitPort(other.IsExplicitPort),
    PrincipalSid(other.PrincipalSid),
    X509FindValue(other.X509FindValue),
    X509StoreName(other.X509StoreName),
    CertFindType(other.CertFindType)
{
}

EndpointCertificateBinding::EndpointCertificateBinding(EndpointCertificateBinding && other) 
    : Port(other.Port),
    IsExplicitPort(other.IsExplicitPort),
    PrincipalSid(move(other.PrincipalSid)),
    X509FindValue(move(other.X509FindValue)),
    X509StoreName(move(other.X509StoreName)),
    CertFindType(other.CertFindType)
{
}

EndpointCertificateBinding const & EndpointCertificateBinding::operator = (EndpointCertificateBinding const & other)
{
    if (this != &other)
    {
        this->Port = other.Port;
        this->IsExplicitPort = other.IsExplicitPort;
        this->PrincipalSid = other.PrincipalSid;
        this->X509FindValue = other.X509FindValue;
        this->X509StoreName = other.X509StoreName;
        this->CertFindType = other.CertFindType;
    }

    return *this;
}

EndpointCertificateBinding const & EndpointCertificateBinding::operator = (EndpointCertificateBinding && other)
{
    if (this != &other)
    {
        this->Port = other.Port;
        this->IsExplicitPort = other.IsExplicitPort;
        this->PrincipalSid = move(other.PrincipalSid);
        this->X509FindValue = move(other.X509FindValue);
        this->X509StoreName = move(other.X509StoreName);
        this->CertFindType = other.CertFindType;
    }

    return *this;
}

bool EndpointCertificateBinding::operator == (EndpointCertificateBinding const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->PrincipalSid, other.PrincipalSid);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->X509FindValue, other.X509FindValue);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->X509StoreName, other.X509StoreName);
    if (!equals) { return equals; }

    equals = this->Port == other.Port;
    if (!equals) { return equals; }

    equals = this->IsExplicitPort == other.IsExplicitPort;
    if (!equals) { return equals; }

    equals = this->CertFindType == other.CertFindType;
    if (!equals) { return equals; }

    return equals;
}

bool EndpointCertificateBinding::operator != (EndpointCertificateBinding const & other) const
{
    return !(*this == other);
}

void EndpointCertificateBinding::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("EndpointCertificateBinding { ");
    w.Write("Port = {0}, ", Port);
    w.Write("IsExplicitPort = {0}", IsExplicitPort);
    w.Write("PrincipalSid = {0}", PrincipalSid);
    w.Write("X509FindValue = {0}", X509FindValue);
    w.Write("X509StoreName = {0}", X509StoreName);
    w.Write("X509FindType = {0}", CertFindType);
    w.Write("}");
}

