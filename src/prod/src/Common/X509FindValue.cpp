// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

static const StringLiteral TraceType("X509FindValue");

_Use_decl_annotations_
ErrorCode X509FindValue::Create(X509FindType::Enum type, LPCWSTR value, SPtr & result)
{
    result.reset();

    if (value == nullptr)
    {
#ifdef PLATFORM_UNIX
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType,
            "Create({0}): input value is null, will match anything", 
            type);

        return ErrorCodeValue::Success;
#else
        switch (type)
        {
        case X509FindType::FindByThumbprint:
            return ErrorCodeValue::InvalidX509Thumbprint;
        case X509FindType::FindBySubjectName:
            return ErrorCodeValue::InvalidSubjectName;
        default:
            return ErrorCodeValue::InvalidArgument;
        }
#endif
    }

    return Create(type, wstring(value), result);
}

_Use_decl_annotations_
ErrorCode X509FindValue::Create(X509FindType::Enum type, std::wstring const & value, SPtr & result)
{
    result.reset();

#ifdef PLATFORM_UNIX
    if (value.empty())
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType,
            "Create({0}): input value is empty, will match anything", 
            type);

        return ErrorCodeValue::Success;
    }
#endif

    switch (type)
    {
        case X509FindType::FindByThumbprint:
        {
            Thumbprint::SPtr thumbprint;
            auto error = Thumbprint::Create(value, thumbprint);
            result = thumbprint;
            return error;
        }

        case X509FindType::FindBySubjectName:
        {
            if (value.empty()) return ErrorCodeValue::InvalidSubjectName;

            SubjectName::SPtr subjectName;
            auto error = SubjectName::Create(value, subjectName);
            result = subjectName;

            if (!error.IsSuccess())
            {
                TraceInfo(
                    TraceTaskCodes::Common,
                    TraceType,
                    "Create({0}, '{1}'): tried as subject name: {2}, Will try as CommonName...",
                    type,
                    value,
                    error);

                CommonName::SPtr commonName;
                error = CommonName::Create(value, commonName);
                result = commonName;
            }

            return error;
        }

        case X509FindType::FindByExtension:
        {
            auto pos = value.find_first_of(L':');
            if (pos == wstring::npos) return ErrorCodeValue::InvalidArgument;

            wstring oid = value.substr(0, pos);
            string oidAnsi;
            StringUtility::UnicodeToAnsi(oid, oidAnsi);
            if (!StringUtility::AreEqualCaseInsensitive(oidAnsi.c_str(), szOID_SUBJECT_ALT_NAME) &&
                !StringUtility::AreEqualCaseInsensitive(oidAnsi.c_str(), szOID_SUBJECT_ALT_NAME2))
                return ErrorCode::FromNtStatus(STATUS_NOT_SUPPORTED);

            wstring extensionValue = value.substr(pos + 1, value.size() - pos - 1);
            X509FindSubjectAltName::SPtr altName;
            auto error = X509FindSubjectAltName::Create(extensionValue, altName);
            result = altName;
            return error;
        }

        default:
            return ErrorCodeValue::InvalidArgument;
    }
}

_Use_decl_annotations_ ErrorCode X509FindValue::Create(
    X509FindType::Enum type,
    std::wstring const & value,
    std::wstring const & secondaryValue,
    SPtr & result)
{
    auto error = Create(type, value, result);
    if (!error.IsSuccess() || secondaryValue.empty()) return error;

    X509FindValue::SPtr secondary;
    error = Create(type, secondaryValue, secondary);
    result->SetSecondary(move(secondary));
    return error;
}

_Use_decl_annotations_ ErrorCode X509FindValue::Create(
    FABRIC_X509_FIND_TYPE type,
    LPCVOID value,
    LPCVOID secondaryValue,
    SPtr & result)
{
    result.reset();

    X509FindType::Enum x509FindType;
    auto error = X509FindType::FromPublic(type, x509FindType);
    if (!error.IsSuccess()) return error;

    if (type == FABRIC_X509_FIND_TYPE_FINDBYEXTENSION)
    {
        if (value == nullptr) return ErrorCodeValue::InvalidArgument;

        X509FindSubjectAltName::SPtr altName;
        error = X509FindSubjectAltName::Create(*((CERT_ALT_NAME_ENTRY const*)value), altName);
        result = move(altName);
        if (!error.IsSuccess() || (secondaryValue == nullptr)) return error;
    }
    else
    {
        error = Create(x509FindType, (LPCWSTR)value, result);
        if (!error.IsSuccess() || (secondaryValue == nullptr)) return error;
    }

    X509FindValue::SPtr secondary;
    error = Create(type, secondaryValue, nullptr, secondary);
    result->SetSecondary(move(secondary));
    return error;
}

_Use_decl_annotations_ ErrorCode X509FindValue::Create(
    std::wstring const & type,
    std::wstring const & value,
    std::wstring const & secondaryValue,
    SPtr & result)
{
    result.reset();

    X509FindType::Enum findType = X509FindType::FindByThumbprint;
    auto error = X509FindType::Parse(type, findType);
    if (!error.IsSuccess()) return error;

    return Create(findType, value, secondaryValue, result);
}

void * X509FindValue::PrimaryValueToPublicPtr(ScopedHeap & heap) const
{
    return (void*)(heap.AddString(PrimaryToString(), true));
}

X509FindValue::~X509FindValue()
{
}

X509FindValue::SPtr const & X509FindValue::Secondary() const
{
    return secondary_;
}

void X509FindValue::SetSecondary(SPtr && secondary)
{
    secondary_ = move(secondary);
}

void X509FindValue::WriteTo(TextWriter & w, FormatOptions const & f) const
{
    w.Write("{0}:", Type());
    OnWriteTo(w, f);
    if (secondary_)
    {
        w.Write(",{0}:", secondary_->Type());
        secondary_->OnWriteTo(w, f);
    }
}

pair<wstring, wstring> X509FindValue::ToStrings() const
{
    pair<wstring, wstring> result;
    result.first = PrimaryToString();

    if (!secondary_) return result;

    result.second = secondary_->PrimaryToString();
    return result;
}

wstring X509FindValue::PrimaryToString() const
{
    wstring str;
    StringWriter w(str);
    OnWriteTo(w, null_format);
    return str;
}

bool X509FindValue::PrimaryValueEqualsTo(X509FindValue const & other) const
{
    return (Type() == other.Type()) && EqualsTo(other);
}

bool X509FindValue::operator == (X509FindValue const & other) const
{
    bool primaryEqual = (Type() == other.Type()) && EqualsTo(other);
    bool secondaryEqual =
        (!secondary_ && !other.secondary_) ||
        (secondary_ && other.secondary_ && secondary_->EqualsTo(*(other.secondary_)));

    return primaryEqual && secondaryEqual;
}

bool X509FindValue::operator != (X509FindValue const & other) const
{
    return !(*this == other);
}
