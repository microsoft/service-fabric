// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class IGateway : public INamingMessageProcessor
    {
    public:
        typedef std::function<Common::AsyncOperationSPtr(
            Transport::MessageUPtr & message,
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & )> BeginHandleGatewayRequestFunction;

        typedef std::function<Common::ErrorCode(
            Common::AsyncOperationSPtr const &, 
            _Out_ Transport::MessageUPtr & )> EndHandleGatewayRequestFunction;

        virtual bool RegisterGatewayMessageHandler(
            Transport::Actor::Enum actor,
            BeginHandleGatewayRequestFunction const& beginFunction,
            EndHandleGatewayRequestFunction const& endFunction) = 0;

        virtual void UnRegisterGatewayMessageHandler(
            Transport::Actor::Enum actor) = 0;

        virtual Common::AsyncOperationSPtr BeginDispatchGatewayMessage(
            Transport::MessageUPtr &&message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDispatchGatewayMessage(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Transport::MessageUPtr & reply) = 0;

    };
}
