// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerApiExecutionResponse::ContainerApiExecutionResponse()
    : StatusCode()
    , ContentType()
    , ContentEncoding()
    , ResponseBody()
{
}

ContainerApiExecutionResponse::ContainerApiExecutionResponse(
    USHORT statusCode,
    wstring const & contentType,
    wstring const & contentEncoding,
    vector<BYTE> && responseBody)
    : StatusCode(statusCode)
    , ContentType(contentType)
    , ContentEncoding(contentEncoding)
    , ResponseBody(move(responseBody))
{
}

ErrorCode ContainerApiExecutionResponse::FromPublicApi(
    FABRIC_CONTAINER_API_EXECUTION_RESPONSE const & apiExecutionResponse)
{
    this->StatusCode = apiExecutionResponse.StatusCode;

    auto error = StringUtility::LpcwstrToWstring2(apiExecutionResponse.ContentType, true, this->ContentType);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(apiExecutionResponse.ContentEncoding, true, this->ContentEncoding);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (apiExecutionResponse.ResponseBodyBufferSize > 0)
    {
        vector<byte> responseBody(
            apiExecutionResponse.ResponseBodyBuffer,
            apiExecutionResponse.ResponseBodyBuffer + apiExecutionResponse.ResponseBodyBufferSize);

        this->ResponseBody = move(responseBody);
    }

    return ErrorCode::Success();
}

void ContainerApiExecutionResponse::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerApiExecutionResponse { ");
    w.Write("StatusCode = {0}", StatusCode);
    w.Write("ContentType = {0}", ContentType);
    w.Write("ContentEncoding = {0}", ContentEncoding);
    w.Write("ResponseBodySize = {0}", ResponseBody.size());
    w.Write("}");
}

