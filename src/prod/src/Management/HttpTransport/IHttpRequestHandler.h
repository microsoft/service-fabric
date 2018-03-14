// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    //
    // This represents a handler registered for an URL suffix.
    //
    class IHttpRequestHandler
    {
    public:
        virtual ~IHttpRequestHandler() {};

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            __in IRequestMessageContextUPtr request,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndProcessRequest(
            __in Common::AsyncOperationSPtr const& operation) = 0;
    };
}
