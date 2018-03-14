// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ProcessGatewayMessageAsyncOperation: public Common::AsyncOperation
    {
        DENY_COPY(ProcessGatewayMessageAsyncOperation)

    public:
        ProcessGatewayMessageAsyncOperation(
            Transport::MessageUPtr &&message,
            EntreeService &owner,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : Common::AsyncOperation(callback, parent)
            , owner_(owner)
            , requestMessage_(move(message))
            , timeout_(timeout)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation, __out Transport::MessageUPtr &message);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

    private:

        void OnComplete(Common::AsyncOperationSPtr const& operation);

        EntreeService& owner_;
        Transport::MessageUPtr requestMessage_;
        IGateway::BeginHandleGatewayRequestFunction beginFunction_;
        IGateway::EndHandleGatewayRequestFunction endFunction_;
        Transport::MessageUPtr replyMessage_;
        Common::TimeSpan timeout_;
    };
}
