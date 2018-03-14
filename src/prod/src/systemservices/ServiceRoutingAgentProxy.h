// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceRoutingAgentProxy;
    typedef std::unique_ptr<ServiceRoutingAgentProxy> ServiceRoutingAgentProxyUPtr;
    typedef std::shared_ptr<ServiceRoutingAgentProxy> ServiceRoutingAgentProxySPtr;

    class ServiceRoutingAgentProxy : public Common::FabricComponent
    {
    public:
        ~ServiceRoutingAgentProxy();

        static ServiceRoutingAgentProxyUPtr Create(
            Federation::NodeInstance const &,
            __in Transport::IpcClient &,
            Common::ComponentRoot const &);

        static ServiceRoutingAgentProxySPtr CreateShared(
            Federation::NodeInstance const &,
            __in Transport::IpcClient &,
            Common::ComponentRoot const &);

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
        Federation::NodeInstance const & get_NodeInstance() const;

        void RegisterMessageHandler(SystemServiceLocation const &, Transport::IpcMessageHandler const &);
        void UnregisterMessageHandler(SystemServiceLocation const &);

        Common::AsyncOperationSPtr BeginSendRequest(
            Transport::MessageUPtr &&, 
            Common::TimeSpan const, 
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndSendRequest(
            Common::AsyncOperationSPtr const &, 
            __out Transport::MessageUPtr &);

        void SendIpcReply(Transport::MessageUPtr &&, Transport::IpcReceiverContext const &);
        void OnIpcFailure(Common::ErrorCode const &, Transport::IpcReceiverContext const &);
        void OnIpcFailure(Common::ErrorCode const &, Transport::IpcReceiverContext const &, Common::ActivityId const & activityId);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        ServiceRoutingAgentProxy(
            Federation::NodeInstance const &,
            __in Transport::IpcClient &,
            Common::ComponentRoot const &);

        class Impl;
        std::unique_ptr<ServiceRoutingAgentProxy::Impl> implUPtr_;
    };
}
