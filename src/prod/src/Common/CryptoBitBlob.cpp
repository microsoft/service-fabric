// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace std;

static StringLiteral const TraceType = "CryptoBitBlob";

CryptoBitBlob::CryptoBitBlob(uint unusedBits) : unusedBits_(unusedBits)
{
}

CryptoBitBlob::CryptoBitBlob(ByteBuffer && data) : data_(move(data))
{
}

CryptoBitBlob::CryptoBitBlob(CRYPT_BIT_BLOB const & blob) : unusedBits_(blob.cUnusedBits)
{
    data_.resize(blob.cbData);
    KMemCpySafe(data_.data(), data_.size(), blob.pbData, blob.cbData);
}

bool CryptoBitBlob::operator == (CryptoBitBlob const & rhs) const
{
    return (data_ == rhs.data_) && (unusedBits_ == rhs.unusedBits_);
}

bool CryptoBitBlob::operator != (CryptoBitBlob const & rhs) const
{
    return !(*this == rhs);
}

ErrorCode CryptoBitBlob::Initialize(wstring const & hexString)
{
    // Normalize the input hex string to something like "3a 4b 2c ..."
    wstring normalizedString = hexString;
    StringUtility::TrimWhitespaces(normalizedString);
    data_.reserve(normalizedString.size() / 2);

    // Add " " for number parsing if needed
    if (normalizedString.find(L" ") == wstring::npos)
    {
        for (int i = (int)normalizedString.size()-2; i >0; )
        {
            normalizedString.insert(i, L" ");
            i -= 2;
        }
    }

    std::wstringstream stringStream(normalizedString);
    while(!stringStream.eof())
    {
        unsigned int byteValue;
        if((stringStream >> std::hex >> byteValue).fail() || (byteValue > numeric_limits<byte>().max()))
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType,
                "CryptoBitBlob::Initialize: Failed to parse {0} as as a hexadecimal string",
                hexString);

            return ErrorCodeValue::InvalidArgument;
        }

        data_.push_back(static_cast<byte>(byteValue));
    }

    return ErrorCodeValue::Success;
}

static const FormatOptions hexByteFormat(2, true, "x");

void CryptoBitBlob::WriteTo(TextWriter & w, FormatOptions const &) const
{
    for (auto b : data_)
    {
        uint64 byteToWrite = b;
        w.WriteNumber(byteToWrite, hexByteFormat, false);
    }
}

#ifdef PLATFORM_UNIX

#else

ErrorCode CryptoBitBlob::InitializeAsSignatureHash(PCCertContext certContext)
{
    data_.resize(64/*512bit*/, 0);

    auto pbData = data_.data();
    auto cbData = (DWORD)data_.size();

    // Load/calculate hash from certificate context
    auto propId = CERT_SIGNATURE_HASH_PROP_ID;
    if (CertGetCertificateContextProperty(certContext, propId, pbData, &cbData) == TRUE)
    {
        if (cbData < data_.size())
        {
            data_.resize(cbData);
        }
        return ErrorCodeValue::Success;
    }

    auto error = ErrorCode::FromWin32Error();
    if (!error.IsWin32Error(ERROR_MORE_DATA))
    {
        TraceError(TraceTaskCodes::Common, TraceType, "InitializeAsSignatureHash: CertGetCertificateContextProperty failed: {0}", error);
        return error;
    }

    data_.resize(cbData);
    pbData = data_.data();

    if (CertGetCertificateContextProperty(certContext, propId, pbData, &cbData) == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType, "InitializeAsSignatureHash: CertGetCertificateContextProperty failed with {0}", error);
        return error;
    }

    return ErrorCodeValue::Success;
}

#endif
