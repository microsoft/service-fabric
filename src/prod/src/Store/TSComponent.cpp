// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data::Utilities;
using namespace ktl;
using namespace ServiceModel;
using namespace std;
using namespace Store;

GlobalWString TSComponent::TypeKeyDelimiter = make_global<std::wstring>(L"+");

TSComponent::TSComponent()
{
}

TSComponent::~TSComponent()
{
}

ErrorCode TSComponent::TryCatchVoid(StringLiteral const & tag, std::function<void(void)> const & callback) const
{
    try
    {
        callback();

        return ErrorCodeValue::Success;
    }
    catch (ktl::Exception const & ex)
    {
        return FromException(tag, ex);
    }
}

ErrorCode TSComponent::TryCatchError(StringLiteral const & tag, std::function<ErrorCode(void)> const & callback) const
{
    try
    {
        return callback();
    }
    catch (ktl::Exception const & ex)
    {
        return FromException(tag, ex);
    }
}

ktl::Awaitable<ErrorCode> TSComponent::TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<void> && awaitable) const
{
    return this->TryCatchAsync(tag, awaitable);
}

ktl::Awaitable<ErrorCode> TSComponent::TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<void> & awaitable) const
{
    try
    {
        co_await awaitable;

        co_return ErrorCodeValue::Success;
    }
    catch (ktl::Exception const & ex)
    {
        co_return FromException(tag, ex);
    }
}

ktl::Awaitable<ErrorCode> TSComponent::TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<ErrorCode> & awaitable) const
{
    try
    {
        co_return co_await awaitable;
    }
    catch (ktl::Exception const & ex)
    {
        co_return FromException(tag, ex);
    }
}

ErrorCode TSComponent::FromNtStatus(StringLiteral const & tag, NTSTATUS status) const
{
    if (NT_SUCCESS(status))
    {
        return ErrorCodeValue::Success;
    }
    else 
    {
        HRESULT hr;
        if (StatusConverter::ToSFHResult(status, hr))
        {
            auto error = ErrorCode::FromHResult(hr);

            this->OnTraceException(wformatString(
                "{0} status=0x{1:x} hr={2} error={3}",
                tag,
                status,
                hr,
                error));

            return error;
        }
        else
        {
            this->OnTraceException(wformatString(
                "{0} status=0x{1:x} hr=unknown",
                tag,
                status,
                hr));

            return ErrorCodeValue::StoreUnexpectedError;
        }
    }
}

void TSComponent::SplitKey(
    KString::SPtr const & kString,
    __out wstring & type,
    __out wstring & key)
{
    wstring input = this->ToWString(kString);
    StringUtility::SplitOnce(input, type, key, *TypeKeyDelimiter);
}

ErrorCode TSComponent::CreateKey(
    wstring const & type,
    wstring const & key,
    __in KAllocator & allocator,
    __out KString::SPtr & result)
{
    if (StringUtility::Contains(type, *TypeKeyDelimiter))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("type='{0}' contains reserved character'{1}'", type, TypeKeyDelimiter));
    }

    auto kKey = wformatString("{0}{1}{2}", type, TypeKeyDelimiter, key);
    auto status = KString::Create(result, allocator, kKey.c_str());

    return this->FromNtStatus("CreateKey", status);
}

std::wstring TSComponent::ToWString(KString::SPtr const & kString)
{
    return wstring(static_cast<wchar_t *>(*kString), kString->Length());
}

std::wstring TSComponent::ToWString(KUri const & kUri)
{
    return wstring(static_cast<wchar_t *>(kUri.Get(KUri::UriFieldType::eRaw)));
}

std::vector<byte> TSComponent::ToByteVector(KBuffer::SPtr const & kBuffer)
{
    auto data = static_cast<byte *>(kBuffer->GetBuffer());

    vector<byte> result;
    result.assign(data, data + kBuffer->QuerySize());

    return move(result);
}

ErrorCode TSComponent::FromException(StringLiteral const & tag, ktl::Exception const & ex) const
{
    return this->FromNtStatus(tag, ex.GetStatus());
}

void TSComponent::OnTraceException(std::wstring const & toTrace) const
{
    WriteInfo(this->GetTraceComponent(), "{0} {1}", this->GetTraceId(), toTrace);
}
