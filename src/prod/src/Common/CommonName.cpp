// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

CommonName::CommonName(wstring const & name) : name_(name)
{
    Initialize();
}

_Use_decl_annotations_
ErrorCode CommonName::Create(std::wstring const & name, SPtr & result)
{
    result = make_shared<CommonName>(name);
    return ErrorCode::Success();
}

void CommonName::Initialize()
{
    certRdnAttr_.pszObjId = szOID_COMMON_NAME;
    certRdnAttr_.dwValueType = CERT_RDN_ANY_TYPE;
    certRdnAttr_.Value.cbData = (DWORD)name_.size() * sizeof(WCHAR);
    certRdnAttr_.Value.pbData = (BYTE*)name_.c_str();

    certRdn_.cRDNAttr = 1;
    certRdn_.rgRDNAttr = &certRdnAttr_;
}

X509FindType::Enum CommonName::Type() const
{
    return X509FindType::FindByCommonName;
}

void const * CommonName::Value() const
{
    return &certRdn_;
}

DWORD CommonName::Flag() const
{
    return CERT_UNICODE_IS_RDN_ATTRS_FLAG;
}

void CommonName::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << name_;
}

void CommonName::OnWriteTo(TextWriter & w, FormatOptions const & f) const
{
    WriteTo(w, f);
}

bool CommonName::EqualsTo(X509FindValue const & other) const
{
    auto rhs = dynamic_cast<CommonName const *>(&other);
    return rhs && StringUtility::AreEqualCaseInsensitive(name_, rhs->name_);
}

bool CommonName::PrimaryValueEqualsTo(CommonName const & other) const
{
    return EqualsTo(other);
}
