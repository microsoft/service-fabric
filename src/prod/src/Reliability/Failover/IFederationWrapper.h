// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceResolver;

    class IFederationWrapper
    {
        DENY_COPY(IFederationWrapper)

    public:

        __declspec(property(get=get_IsShutdown)) bool IsShutdown;
        virtual bool get_IsShutdown() const = 0;

        __declspec (property(get=get_Instance)) Federation::NodeInstance const& Instance;
        virtual Federation::NodeInstance const& get_Instance() const = 0 ;

        // Used for tracing
        __declspec (property(get=get_InstanceIdStr)) std::wstring const& InstanceIdStr;
        virtual std::wstring const & get_InstanceIdStr() const = 0;

        virtual void SendToFM(Transport::MessageUPtr && message) = 0;
        virtual void SendToFMM(Transport::MessageUPtr && message) = 0;
        virtual void SendToNode(Transport::MessageUPtr && message, Federation::NodeInstance const & target) = 0;
        virtual void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Transport::MessageUPtr && reply) = 0;
        virtual void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Common::ErrorCode const & error) = 0;

        virtual Common::AsyncOperationSPtr BeginRequestToFMM(
            Transport::MessageUPtr && message,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const = 0;
        
        virtual Common::ErrorCode EndRequestToFMM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const = 0;

        virtual Common::AsyncOperationSPtr BeginRequestToFM(
            Transport::MessageUPtr && message,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndRequestToFM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) = 0;


        virtual ~IFederationWrapper() {}

    protected:
        IFederationWrapper() { }
    };

}
