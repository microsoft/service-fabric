// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    class HttpClientImpl
        : public Common::RootedObject
        , public IHttpClient
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
    public:

        //
        // This should take in client settings, when we support certs
        //
        static Common::ErrorCode CreateHttpClient(
            std::wstring const &clientId,
            Common::ComponentRoot const &root,
            __out IHttpClientSPtr &httpClient);

        //
        // Default behavior for an HTTP Request is to follow redirects
        // enableWinauthAutoLogon: This can be used in conjunction with client certificate.
        // Depending on the server's auth policy, either one or both windows auth and certificate based auth will be used. 
        //
        Common::ErrorCode CreateHttpRequest(
            std::wstring const &requestUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            KAllocator &allocator,
            __out IHttpClientRequestSPtr &clientRequest,
            bool allowRedirects = true,
            bool enableCookies = true,
            bool enableWinauthAutoLogon = false);

        Common::ErrorCode CreateHttpRequest(
            std::wstring const &requestUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            __out IHttpClientRequestSPtr &clientRequest,
            bool allowRedirects = true,
            bool enableCookies = true,
            bool enableWinauthAutoLogon = false);

    private:
        HttpClientImpl(Common::ComponentRoot const &root)
            : RootedObject(root)
        {
        }

        Common::ErrorCode Initialize(
            std::wstring const &clientId);

        KHttpClient::SPtr kHttpClient_;
    };
}
