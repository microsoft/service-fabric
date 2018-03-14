// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

CertificateAccessDescription::CertificateAccessDescription() 
    : certificate_(),
    userSids_(),
    accessMask_()
{
}

CertificateAccessDescription::CertificateAccessDescription(
    SecretsCertificateDescription const & certificate,
    vector<wstring> const & userSids,
    DWORD accessMask) : certificate_(certificate),
    userSids_(userSids),
    accessMask_(accessMask)
{
}

CertificateAccessDescription::CertificateAccessDescription(
    CertificateAccessDescription const & other) 
    : certificate_(other.certificate_),
    userSids_(other.userSids_),
    accessMask_(other.accessMask_)
{
}

CertificateAccessDescription::CertificateAccessDescription(CertificateAccessDescription && other) 
    : certificate_(move(other.certificate_)),
    userSids_(move(other.userSids_)),
    accessMask_(move(other.accessMask_))
{
}

CertificateAccessDescription const & CertificateAccessDescription::operator = (CertificateAccessDescription const & other)
{
    if (this != &other)
    {
        this->certificate_ = other.certificate_;
        this->userSids_ = other.userSids_;
        this->accessMask_ = other.accessMask_;
    }

    return *this;
}

CertificateAccessDescription const & CertificateAccessDescription::operator = (CertificateAccessDescription && other)
{
    if (this != &other)
    {
        this->certificate_ = move(other.certificate_);
        this->userSids_ = move(other.userSids_);
        this->accessMask_ = other.accessMask_;
    }

    return *this;
}

void CertificateAccessDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("CertificateAccessDescription { ");
    w.Write("certificate = {0}, ", certificate_);
    
    w.Write("UserSids { ");
    for(auto iter = userSids_.begin(); iter != userSids_.end(); ++iter)
    {
        w.Write("userSid = {0}", *iter);
    }
    w.Write("}");

    w.Write("AccessMask = {0}", accessMask_);

    w.Write("}");
}

