// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ContainerApiResult::ContainerApiResult(unsigned status, wstring && contentType, wstring && contentEncoding, wstring && body)
    : status_(status)
    , contentType_(move(contentType))
    , contentEncoding_(move(contentEncoding))
    , body_(move(body))
{
}

void ContainerApiResult::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ContainerApiResult::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ContainerApiResult&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ContainerApiResponse::ContainerApiResponse(unsigned status, wstring && contentType, wstring && contentEncoding, wstring && body)
    : result_(status, move(contentType), move(contentEncoding), move(body))
{
}

wstring ContainerApiResponse::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ContainerApiResponse&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}
