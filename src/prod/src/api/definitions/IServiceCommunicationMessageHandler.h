// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{

    class IServiceCommunicationMessageHandler
    {
    public:

        virtual ~IServiceCommunicationMessageHandler() {};

        virtual void Initialize()=0;

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            std::wstring const & clientId,
            Transport::MessageUPtr && message,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &  operation,
            Transport::MessageUPtr & reply) = 0;

        virtual Common::ErrorCode HandleOneWay(
            std::wstring const & clientId,
            Transport::MessageUPtr && message) = 0;

    };
}
