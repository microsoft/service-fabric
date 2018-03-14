// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Api
{

    class ICommunicationMessageSender
    {
    public:

        virtual Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && message,
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndRequest(
            Common::AsyncOperationSPtr const &  operation,
            Transport::MessageUPtr & reply) = 0;

        virtual Common::ErrorCode SendOneWay(Transport::MessageUPtr && message) =0;

        virtual ~ICommunicationMessageSender() {};

    };

}
