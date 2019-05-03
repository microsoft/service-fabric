// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpClient
{
    class IHttpClient
    {
    public:
        virtual ~IHttpClient() {};

        virtual Common::ErrorCode CreateHttpRequest(
            std::wstring const &requestUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            KAllocator &allocator,
            __out IHttpClientRequestSPtr &clientRequest,
            bool allowRedirects = true,
            bool enableCookies = true,
            bool enableWinauthAutoLogon = false) = 0;

        virtual Common::ErrorCode CreateHttpRequest(
            std::wstring const &requestUri,
            Common::TimeSpan const &connectTimeout,
            Common::TimeSpan const &sendTimeout,
            Common::TimeSpan const &receiveTimeout,
            __out IHttpClientRequestSPtr &clientRequest,
            bool allowRedirects = true,
            bool enableCookies = true,
            bool enableWinauthAutoLogon = false) = 0;
    };
}
