// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class  IServiceConnectionHandler
    {
    public:
        virtual ~IServiceConnectionHandler() {};

        virtual Common::AsyncOperationSPtr BeginProcessConnect(
            Api::IClientConnectionPtr clientConnection,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndProcessConnect(
            Common::AsyncOperationSPtr const &  operation) = 0;

        virtual Common::AsyncOperationSPtr BeginProcessDisconnect(
            std::wstring const & clientId,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndProcessDisconnect(
            Common::AsyncOperationSPtr const &  operation) = 0;

    };
}
