// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace std;

static StringLiteral const TraceType = "Security";

Thumbprint::Thumbprint() : value_(), certChainShouldBeVerified_(false)
{
    buffer_.reserve(HASH_SIZE_DEFAULT);
}

Thumbprint::Thumbprint(Thumbprint const & other) : buffer_(other.buffer_), certChainShouldBeVerified_(other.certChainShouldBeVerified_)
{
    SetBlobValue();
}

Thumbprint::Thumbprint(Thumbprint && other) : buffer_(move(other.buffer_)), certChainShouldBeVerified_(other.certChainShouldBeVerified_)
{
    SetBlobValue();
    other.SetBlobValue();
}

X509Identity::Type Thumbprint::IdType() const
{
    return X509Identity::Thumbprint;
}

_Use_decl_annotations_
ErrorCode Thumbprint::Create(std::wstring const & str, SPtr & result)
{
    result = make_shared<Thumbprint>();
    auto error = result->Initialize(str);
    if (!error.IsSuccess())
    {
        result.reset();
    }

    return error;
}

_Use_decl_annotations_
ErrorCode Thumbprint::Create(PCCertContext certContext, SPtr & result)
{
    result = make_shared<Thumbprint>();
    auto error = result->Initialize(certContext);
    if (!error.IsSuccess())
    {
        result.reset();
    }
    
    return error;
}

Thumbprint & Thumbprint::operator = (Thumbprint const & other)
{
    if (this == &other)
    {
        return *this;
    }

    X509FindValue::operator=(other);
    buffer_ = other.buffer_;
    SetBlobValue();
    return *this;
}

Thumbprint & Thumbprint::operator = (Thumbprint && other)
{
    if (this == &other)
    {
        return *this;
    }

    X509FindValue::operator=(move(other));
    buffer_ = move(other.buffer_);
    SetBlobValue();
    other.SetBlobValue();
    return *this;
}

bool Thumbprint::EqualsTo(X509FindValue const & other) const
{
    auto rhs = dynamic_cast<Thumbprint const*>(&other);
    return rhs && (*this == *rhs);
}

bool Thumbprint::PrimaryValueEqualsTo(Thumbprint const & other) const
{
    return EqualsTo(other);
}

bool Thumbprint::operator == (Thumbprint const & rhs) const
{
    // X509FindValue::secondary_ is only meaningful for comparing as X509FindValue,
    // thus ignored here, X509FindValue_::operator== covers X509FindValue_::secondary_
    return (buffer_.size() == rhs.buffer_.size()) && (::memcmp(buffer_.data(), rhs.buffer_.data(), buffer_.size()) == 0);
}

bool Thumbprint::operator != (Thumbprint const & rhs) const
{
    return !(*this == rhs);
}

bool Thumbprint::operator < (Thumbprint const & rhs) const
{
    return
        (buffer_.size() < rhs.buffer_.size()) ||
        ((buffer_.size() == rhs.buffer_.size()) && (::memcmp(buffer_.data(), rhs.buffer_.data(), buffer_.size()) < 0));
}

bool Thumbprint::operator == (X509Identity const & rhs) const
{
    return (IdType() == rhs.IdType()) && (*this == rhs);
}

bool Thumbprint::operator < (X509Identity const & rhs) const
{
    return
        (IdType() < rhs.IdType()) ||
        ((IdType() == rhs.IdType()) && (*this < (Thumbprint const&)rhs));
}

#if defined(PLATFORM_UNIX)
ErrorCode Thumbprint::Initialize(PCCertContext certContext)
{
    if(certContext == NULL)
    {
        TraceError(TraceTaskCodes::Common, TraceType, "Thumbprint::Initialize: certContext is NULL.");
        return ErrorCodeValue::ArgumentNull;
    }

    buffer_.resize(HASH_SIZE_DEFAULT, 0);
    SetBlobValue();

    // Load/calculate hash from certificate context
    auto error = LinuxCryptUtil().GetSHA1Thumbprint((X509*)certContext, buffer_);
    if (!error.IsSuccess()) 
    { 
        TraceError(TraceTaskCodes::Common, TraceType, "Thumbprint::Initialize: LinuxCryptUtil::GetSHA1Thumbprint() failed: {0}", error);
        return error; 
    }

    value_.pbData = buffer_.data();
    value_.cbData = buffer_.size();
    return ErrorCode::Success();
}
#else
ErrorCode Thumbprint::Initialize(PCCertContext certContext)
{
    buffer_.resize(HASH_SIZE_DEFAULT, 0);
    SetBlobValue();

    // Load/calculate hash from certificate context
    auto propId = CERT_HASH_PROP_ID;
    if (CertGetCertificateContextProperty(certContext, propId, value_.pbData, &(value_.cbData)) == TRUE)
    {
        if (value_.cbData < buffer_.size())
        {
            buffer_.resize(value_.cbData);
        }
        return ErrorCodeValue::Success;
    }

    auto error = ErrorCode::FromWin32Error();
    if (!error.IsWin32Error(ERROR_MORE_DATA))
    {
        TraceError(TraceTaskCodes::Common, TraceType, "Thumbprint::Initialize: CertGetCertificateContextProperty failed: {0}", error);
        return error;
    }

    buffer_.resize(value_.cbData);
    value_.pbData = buffer_.data();

    if (CertGetCertificateContextProperty(certContext, propId, value_.pbData, &(value_.cbData)) == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType, "Thumbprint::Initialize: CertGetCertificateContextProperty failed with {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}
#endif

bool Thumbprint::CertChainShouldBeVerified() const
{
    return certChainShouldBeVerified_;
}

ErrorCode Thumbprint::Initialize(wstring const & inputHashString)
{
    vector<wstring> strings;
    StringUtility::Split<wstring>(inputHashString, strings, L"?", true);
    if (strings.empty()) return ErrorCodeValue::InvalidX509Thumbprint;

    if (strings.size() > 1)
    {
        StringUtility::TrimWhitespaces(strings[1]);
        certChainShouldBeVerified_ = StringUtility::AreEqualCaseInsensitive(strings[1], L"true");
    }

    // Parse from input string, examples:
    // "2855e9bbedc65d3dc82c7335d1ec074586b09006" or "28 55 e9 bb ed c6 5d 3d c8 2c 73 35 d1 ec 07 45 86 b0 90 06"
    wstring & normalizedHashString = strings.front();
    StringUtility::TrimWhitespaces(normalizedHashString);

    // Add " " for number parsing if needed
    if (normalizedHashString.find(L" ") == wstring::npos)
    {
        for (int i = (int)normalizedHashString.size()-2; i >0; )
        {
            normalizedHashString.insert(i, L" ");
            i -= 2;
        }
    }

    std::wstringstream stringStream(normalizedHashString);
    while(!stringStream.eof())
    {
        unsigned int hashByte;
        if((stringStream >> std::hex >> hashByte).fail() || (hashByte > numeric_limits<byte>().max()))
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType,
                "Thumbprint::Initialize: Failed to parse {0} as certificate thumbprint, the format should be '2855e9bbedc65d3dc82c7335d1ec074586b09006' or '28 55 e9 bb ed c6 5d 3d c8 2c 73 35 d1 ec 07 45 86 b0 90 06'",
                inputHashString);

            return ErrorCodeValue::InvalidX509Thumbprint;
        }

        buffer_.push_back(static_cast<byte>(hashByte));
    }

    SetBlobValue();
    return ErrorCodeValue::Success;
}

void Thumbprint::SetBlobValue()
{
    value_.cbData = (ULONG)buffer_.size();
    value_.pbData = buffer_.data();
}

static const FormatOptions hexByteFormat(2, true, "x");

void Thumbprint::WriteTo(TextWriter & w, FormatOptions const &) const
{
    for (DWORD i = 0; i < value_.cbData; ++i)
    {
        uint64 byteToWrite = value_.pbData[i];
        w.WriteNumber(byteToWrite, hexByteFormat, false);
    }
}

void Thumbprint::OnWriteTo(TextWriter & w, FormatOptions const & f) const
{
    WriteTo(w, f);
}

X509FindType::Enum Thumbprint::Type() const
{
    return X509FindType::FindByThumbprint;
}

void const * Thumbprint::Value() const
{
    return &value_;
}

BYTE const * Thumbprint::Hash() const
{
    return value_.pbData;
}

DWORD Thumbprint::HashSizeInBytes() const
{
    return value_.cbData;
}
