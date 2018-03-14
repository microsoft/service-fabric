// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

static StringLiteral const TraceType("Security");

X509FindSubjectAltName::X509FindSubjectAltName() : certAltName_()
{
}

_Use_decl_annotations_
ErrorCode X509FindSubjectAltName::Create(wstring const & nameTypeAndValue, SPtr & result)
{
    result = make_shared<X509FindSubjectAltName>();
    auto error = result->Initialize(nameTypeAndValue);
    if (!error.IsSuccess())
    {
        result.reset();
    }

    return error;
}

_Use_decl_annotations_
ErrorCode X509FindSubjectAltName::Create(CERT_ALT_NAME_ENTRY const & certAltName, SPtr & result)
{
    result = make_shared<X509FindSubjectAltName>();
    auto error = result->Initialize(certAltName);
    if (!error.IsSuccess())
    {
        result.reset();
    }

    return error;
}

ErrorCode X509FindSubjectAltName::Initialize(CERT_ALT_NAME_ENTRY const & certAltName)
{
    certAltName_.dwAltNameChoice = certAltName.dwAltNameChoice;
    if (certAltName_.dwAltNameChoice != CERT_ALT_NAME_DNS_NAME)
        return ErrorCode::FromNtStatus(STATUS_NOT_SUPPORTED);

    wstring dnsName(certAltName.pwszDNSName);
    nameBuffer_.resize((dnsName.size() + 1) * sizeof(WCHAR));
    KMemCpySafe(nameBuffer_.data(), nameBuffer_.size(), certAltName.pwszDNSName, nameBuffer_.size());

    certAltName_.pwszDNSName = (LPWSTR)nameBuffer_.data();
    return ErrorCode::Success();
}

ErrorCode X509FindSubjectAltName::Initialize(wstring const & nameTypeAndValue)
{
    auto pos = nameTypeAndValue.find_first_of(L'=');
    if (pos == wstring::npos) return ErrorCodeValue::InvalidArgument;

    wstring nameType = nameTypeAndValue.substr(0, pos);
    wstring nameValue = nameTypeAndValue.substr(pos + 1, nameTypeAndValue.size() - pos - 1);
    StringUtility::TrimWhitespaces(nameValue);

    if ((wstringstream(nameType) >> certAltName_.dwAltNameChoice).fail())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType,
            "X509FindSubjectAltName::Initialize: Failed to parse '{0}' as alternative name type",
            nameType);

        return ErrorCodeValue::InvalidArgument;
    }

    certAltName_.pwszDNSName = (LPWSTR)(nameValue.c_str());
    return Initialize(certAltName_);
}

X509FindType::Enum X509FindSubjectAltName::Type() const
{
    return X509FindType::FindByExtension;
}

void const * X509FindSubjectAltName::Value() const
{
    return &certAltName_;
}

void X509FindSubjectAltName::WriteTo(TextWriter & w, FormatOptions const &) const
{
    // The output must be parsable by X509FindValue::Create()
    w << szOID_SUBJECT_ALT_NAME2 << L':' << certAltName_.dwAltNameChoice << L'=' << certAltName_.pwszDNSName;
}

void X509FindSubjectAltName::OnWriteTo(TextWriter & w, FormatOptions const & f) const
{
    WriteTo(w, f);
}

bool X509FindSubjectAltName::EqualsTo(X509FindValue const & other) const
{
    auto rhs = dynamic_cast<X509FindSubjectAltName const *>(&other);
    if (!rhs) return false;

    switch (certAltName_.dwAltNameChoice)
    {
    case CERT_ALT_NAME_DNS_NAME:
        return StringUtility::AreEqualCaseInsensitive(
            certAltName_.pwszDNSName,
            ((X509FindSubjectAltName const &)other).certAltName_.pwszDNSName);

    default:
        return false;
    }
}

bool X509FindSubjectAltName::PrimaryValueEqualsTo(X509FindSubjectAltName const & other) const
{
    return EqualsTo(other);
}

void* X509FindSubjectAltName::PrimaryValueToPublicPtr(Common::ScopedHeap & heap) const
{
    ReferenceArray<byte> bufferRPtr = heap.AddArray<byte>(nameBuffer_.size());
    KMemCpySafe(bufferRPtr.GetRawArray(), bufferRPtr.GetCount(), nameBuffer_.data(), nameBuffer_.size());

    ReferencePointer<CERT_ALT_NAME_ENTRY> certAltNameRPtr = heap.AddItem<CERT_ALT_NAME_ENTRY>();
    Invariant(certAltName_.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME);
    certAltNameRPtr->dwAltNameChoice = certAltName_.dwAltNameChoice;
    certAltNameRPtr->pwszDNSName = (LPWSTR)bufferRPtr.GetRawArray();

    return certAltNameRPtr.GetRawPointer();
}
