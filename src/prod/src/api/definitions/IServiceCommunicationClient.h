// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{

    class  IServiceCommunicationClient : public ICommunicationMessageSender
    {
    public:
        virtual void AbortClient() = 0;

        virtual void CloseClient() = 0;

        virtual Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const &  operation) = 0;

        virtual Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndClose(
            Common::AsyncOperationSPtr const &  operation) = 0;
    };
}
