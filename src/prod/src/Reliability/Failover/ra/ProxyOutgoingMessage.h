// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ProxyOutgoingMessage
        {
            DENY_COPY(ProxyOutgoingMessage);

        public:
            ProxyOutgoingMessage(
                Transport::MessageUPtr && message,
                FailoverUnitProxyWPtr && failoverUnitProxyWPtr)
                :message_(std::move(message)),
                 failoverUnitProxyWPtr_(std::move(failoverUnitProxyWPtr))
            {
            }

             __declspec (property(get=get_TransportMessage)) Transport::MessageUPtr const& TransportMessage;
            Transport::MessageUPtr const& get_TransportMessage() const { return message_; }

            inline bool ShouldResend()
            {
                FailoverUnitProxySPtr failoverUnitProxySPtr = failoverUnitProxyWPtr_.lock();

                if(!failoverUnitProxySPtr)
                {
                    return false;
                }

                return failoverUnitProxySPtr->ShouldResendMessage(message_);
            }

        private:
            Transport::MessageUPtr message_;
            FailoverUnitProxyWPtr failoverUnitProxyWPtr_;
        };
    }
}

