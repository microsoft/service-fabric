// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

static StringLiteral const TraceType("Security");

SubjectName::SubjectName(wstring const & name) : name_(name)
{
}

_Use_decl_annotations_
ErrorCode SubjectName::Create(std::wstring const & name, SPtr & result)
{
    result = make_shared<SubjectName>(name);
    auto error = result->Initialize();
    if (!error.IsSuccess())
    {
        result.reset();
    }

    return error;
}

#if defined(PLATFORM_UNIX)
ErrorCode SubjectName::Initialize()
{
    if (name_.empty())
    {
        TraceWarning(TraceTaskCodes::Common, TraceType, "SubjectName::Initialize: Subject name should not be empty");
        return ErrorCodeValue::InvalidSubjectName;
    }

    auto error = LinuxCryptUtil::GetKeyValPairsFromSubjectName(name_, nameBlob_);

    if (!error.IsSuccess())
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType, 
            "SubjectName::Initialize: '{0}' is not a valid X500 subject name: {1}",
            name_, error);

        return error;
    }

    return ErrorCode::Success();
}
#else
ErrorCode SubjectName::Initialize()
{
    if (name_.empty())
    {
        TraceWarning(TraceTaskCodes::Common, TraceType, "SubjectName::Initialize: Subject name should not be empty");
        return ErrorCodeValue::InvalidSubjectName;
    }

    nameBlob_.cbData = 0;
    nameBlob_.pbData = nullptr;
    CertStrToName(X509_ASN_ENCODING, name_.c_str(), CERT_X500_NAME_STR, nullptr, nameBlob_.pbData, &nameBlob_.cbData, nullptr);

    nameBuffer_.resize(nameBlob_.cbData);
    nameBlob_.pbData = nameBuffer_.data();
    if (!CertStrToName(X509_ASN_ENCODING, name_.c_str(), CERT_X500_NAME_STR, nullptr, nameBlob_.pbData, &nameBlob_.cbData, nullptr))
    {
        auto error = microsoft::GetLastErrorCode();
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType, 
            "SubjectName::Initialize: '{0}' is not a valid X500 subject name: {1}:{2}",
            name_, error.value(), error.message());
        return ErrorCodeValue::InvalidSubjectName;
    }

    return ErrorCode::Success();
}
#endif

X509FindType::Enum SubjectName::Type() const
{
    return X509FindType::FindBySubjectName;
}

void const * SubjectName::Value() const
{
    return &nameBlob_;
}

wstring const & SubjectName::Name() const
{
    return name_;
}

void SubjectName::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << name_;
}

void SubjectName::OnWriteTo(TextWriter & w, FormatOptions const & f) const
{
    WriteTo(w, f);
}

bool SubjectName::EqualsTo(X509FindValue const & other) const
{
    auto rhs = dynamic_cast<SubjectName const *>(&other);

#if defined(PLATFORM_UNIX)
    return rhs && LinuxCryptUtil::MapCompare(nameBlob_, rhs->nameBlob_);
#else
    return rhs && StringUtility::AreEqualCaseInsensitive(name_, rhs->name_);
#endif
}

bool SubjectName::PrimaryValueEqualsTo(SubjectName const & other) const
{
    return EqualsTo(other);
}
