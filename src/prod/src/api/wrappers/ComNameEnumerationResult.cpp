// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Api;
using namespace Naming;

ComNameEnumerationResult::ComNameEnumerationResult(
    EnumerateSubNamesResult && enumerationResult)
    : enumerationResult_(std::move(enumerationResult)),
    buffer_()
{
    std::vector<LPCWSTR> buffer;
    for (std::wstring const & item : enumerationResult_.SubNames)
    {
        buffer.push_back(item.c_str());
    }
    buffer_ = std::move(buffer);
}

EnumerateSubNamesToken const & ComNameEnumerationResult::get_ContinuationToken()
{
    return enumerationResult_.ContinuationToken;
}

FABRIC_ENUMERATION_STATUS STDMETHODCALLTYPE ComNameEnumerationResult::get_EnumerationStatus()
{
    return enumerationResult_.FABRIC_ENUMERATION_STATUS;
}

HRESULT STDMETHODCALLTYPE ComNameEnumerationResult::GetNames(
    __out ULONG * itemCount,
    __out LPCWSTR const ** bufferedItems)
{
    if ((itemCount == NULL) || (bufferedItems == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    *itemCount = static_cast<ULONG>(buffer_.size());
    *bufferedItems = buffer_.data();
    return ComUtility::OnPublicApiReturn(S_OK);
}
