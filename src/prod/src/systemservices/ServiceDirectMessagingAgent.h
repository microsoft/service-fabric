// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceDirectMessagingAgent;
    typedef std::shared_ptr<ServiceDirectMessagingAgent> ServiceDirectMessagingAgentSPtr;

    class ServiceDirectMessagingAgent : public Common::FabricComponent
    {
    public:
        static ServiceDirectMessagingAgentSPtr CreateShared(
            Federation::NodeInstance const &,
            std::wstring const & hostAddress,
            Common::ComponentRoot const &);

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
        Federation::NodeInstance const & get_NodeInstance() const;

        __declspec(property(get=get_HostAddress)) std::wstring const & HostAddress;
        std::wstring const & get_HostAddress() const;

        void RegisterMessageHandler(SystemServiceLocation const &, Transport::DemuxerMessageHandler const &);
        void UnregisterMessageHandler(SystemServiceLocation const &);

        void SendDirectReply(Transport::MessageUPtr &&, Transport::ReceiverContext const &);
        void OnDirectFailure(Common::ErrorCode const &, Transport::ReceiverContext const &);
        void OnDirectFailure(Common::ErrorCode const &, Transport::ReceiverContext const &, Common::ActivityId const & activityId);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        ServiceDirectMessagingAgent(
            Federation::NodeInstance const &,
            std::wstring const & hostAddress,
            Common::ComponentRoot const &);

        class Impl;
        std::unique_ptr<ServiceDirectMessagingAgent::Impl> implUPtr_;
    };
}
