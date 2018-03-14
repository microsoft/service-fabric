// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceCommunicationListener
    {
    public:
        virtual Common::AsyncOperationSPtr BeginOpen(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out std::wstring & serviceAddress) = 0;

        virtual Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual void AbortListener() = 0;

          virtual ~IServiceCommunicationListener() {};

    };
}
