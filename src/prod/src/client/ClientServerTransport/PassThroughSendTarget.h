// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class PassThroughSendTarget : public Transport::ISendTarget
    {
        DENY_COPY(PassThroughSendTarget)
    public:
        virtual ~PassThroughSendTarget() {}

        PassThroughSendTarget(
            std::wstring address,
            std::wstring id,
            ClientServerPassThroughTransport &owningTransport)
            : address_(address)
            , id_(id)
            , owningTransport_(owningTransport)
        {}

        std::wstring const & Address() const { return address_; }

        std::wstring const & LocalAddress() const { Common::Assert::CodingError("Unsupported API"); }

        std::wstring const & Id() const { return id_; }
        std::wstring const & TraceId() const override { return id_; }

        bool IsAnonymous() const { return false; }

        size_t ConnectionCount() const { return 1; }

        Common::AsyncOperationSPtr BeginReceiveNotification(
            Transport::MessageUPtr &&message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);       
        Common::ErrorCode EndReceiveNotification(
            Common::AsyncOperationSPtr const &operation,
            ClientServerReplyMessageUPtr &reply);

    private:
        std::wstring address_;
        std::wstring id_;
        ClientServerPassThroughTransport &owningTransport_;
    };
}
