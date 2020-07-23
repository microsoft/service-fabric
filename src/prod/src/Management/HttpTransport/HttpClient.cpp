// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpCommon;
using namespace HttpClient;

StringLiteral const TraceType("HttpClient");

ErrorCode HttpClientImpl::CreateHttpClient(
    wstring const &clientId,
    ComponentRoot const &root,
    __out IHttpClientSPtr &httpClient)
{
    auto httpClientImpl = shared_ptr<HttpClientImpl>(new HttpClientImpl(root));

    auto error = httpClientImpl->Initialize(
        clientId);

    if (error.IsSuccess())
    {
        httpClient = move(httpClientImpl);
    }

    return error;
}

ErrorCode HttpClientImpl::Initialize(
    wstring const &clientId)
{
    KStringView clientIdStr(clientId.c_str());
    auto status = KHttpClient::Create(
        clientIdStr,
        GetSFDefaultPagedKAllocator(),
        kHttpClient_);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Creating KHttpClient failed with {0}",
            status);

        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode HttpClientImpl::CreateHttpRequest(
    wstring const &requestUri,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    KAllocator &allocator,
    __out IHttpClientRequestSPtr &clientRequest,
    bool allowRedirects,
    bool enableCookies,
    bool enableWinauthAutoLogon)
{
    return ClientRequest::CreateClientRequest(
            kHttpClient_, 
            requestUri, 
            allocator,
            connectTimeout,
            sendTimeout,
            receiveTimeout,
            allowRedirects,
            enableCookies,
            enableWinauthAutoLogon,
            clientRequest);
}

ErrorCode HttpClientImpl::CreateHttpRequest(
    wstring const &requestUri,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    __out IHttpClientRequestSPtr &clientRequest,
    bool allowRedirects,
    bool enableCookies,
    bool enableWinauthAutoLogon)
{
    return ClientRequest::CreateClientRequest(
        kHttpClient_,
        requestUri,
        HttpUtil::GetKtlAllocator(),
        connectTimeout,
        sendTimeout,
        receiveTimeout,
        allowRedirects,
        enableCookies,
        enableWinauthAutoLogon,
        clientRequest);
}
