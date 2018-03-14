// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceRoutingAgentMessage
    {
    public:
        // There is one common IPC failure reply for the IPC client
        // to recognize failures in a generic way
        //
        static Common::GlobalWString IpcFailureAction;

        // For services using the Routing Agent
        //
        static Common::GlobalWString ServiceRouteRequestAction;

        // For services registered directly with federation
        //
        static Common::GlobalWString ForwardMessageAction;

        // For services registered directly with federation
        //
        static Common::GlobalWString ForwardToFileStoreMessageAction;

        // For services registered with Routing Agent
        //
        static Common::GlobalWString ForwardToTvsMessageAction;

        static Transport::MessageUPtr CreateIpcFailureMessage(IpcFailureBody const & body, Common::ActivityId const & activityId)
        { 
            return CreateMessage(IpcFailureAction, body, activityId);
        }

        static void WrapForIpc(__inout Transport::Message &);

        static void WrapForIpc(__inout Transport::Message &, Transport::Actor::Enum const, std::wstring const & action);

        static Common::ErrorCode UnwrapFromIpc(__inout Transport::Message &);
        
        static void WrapForRoutingAgent(__inout Transport::Message &, ServiceModel::ServiceTypeIdentifier const & typeId);

        static void WrapForRoutingAgent(__inout Client::ClientServerRequestMessage &, ServiceModel::ServiceTypeIdentifier const & typeId);

        static Common::ErrorCode RewrapForRoutingAgentProxy(__inout Transport::Message &, ServiceRoutingAgentHeader const &);

        static void WrapForForwarding(__inout Transport::Message &);

        static void WrapForForwardingToFileStoreService(__inout Transport::Message &);

        static void WrapForForwardingToTokenValidationService(__inout Transport::Message &);

        static void WrapForForwarding(__inout Client::ClientServerRequestMessage &);

        static void WrapForForwardingToFileStoreService(__inout Client::ClientServerRequestMessage &);

        static void WrapForForwardingToTokenValidationService(__inout Client::ClientServerRequestMessage &);

        static Common::ErrorCode ValidateIpcReply(__in Transport::Message & );

        // Sets ServiceRoutingAgentHeader and ForwardMessageHeader, to prepare to route a message to a
        // service via the ServiceRoutingAgent by way of ForwardToServiceOperation.
        static void SetForwardingHeaders(
            __inout Transport::Message &,
            Transport::Actor::Enum const,
            std::wstring const &,
            ServiceModel::ServiceTypeIdentifier const &);

    private:

        template <class TBody>
        static Transport::MessageUPtr CreateMessage(std::wstring const & action, TBody const & body, Common::ActivityId const & activityId)
        {
            Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));

            SetRoutingAgentHeaders(*message, action);

            if(activityId.Guid != Common::Guid::Empty())
            {
                message->Headers.Add(Transport::FabricActivityHeader(activityId));
            }

            return message;
        }

        static Transport::MessageUPtr CreateMessage(std::wstring const & action);

        static void SetRoutingAgentHeaders(__inout Transport::Message &, std::wstring const &);
        static void SetRoutingAgentHeaders(__inout Client::ClientServerRequestMessage &, std::wstring const &);
    };
}
