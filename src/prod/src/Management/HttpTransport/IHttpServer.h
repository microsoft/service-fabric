// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    //
    // Request Handler's can register to receive requests to the specified url suffix,
    // following the root URL that the http server is listening on. The URL the server
    // is listening on is got from the config, specified via the cluster manifest.
    // 
    // urlSuffixes are of the form /<component A>/<component B>/. Wild card matching 
    // is supported when a component name is specified as '*' (i.e) /*/<component A>/
    // Note: The suffix path for handler registration shouldnt have QueryString - '?<>' 
    // parameter.
    class IHttpServer
    {
    public:
        virtual ~IHttpServer() {};

        virtual Common::ErrorCode RegisterHandler(
            __in std::wstring const& urlSuffix,
            __in IHttpRequestHandlerSPtr const& handler) = 0;

        virtual Common::ErrorCode UnRegisterHandler(
            __in std::wstring const& urlSuffix,
            __in IHttpRequestHandlerSPtr const& handler) = 0;

    };
}
