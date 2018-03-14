// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{    
    class ServiceDirectMessagingAgentMessage
    {
    public:
        static Common::GlobalWString DirectMessagingFailureAction;
        static Common::GlobalWString DirectMessagingAction;

        static Transport::MessageUPtr CreateFailureMessage(Common::ErrorCode const & error, Common::ActivityId const & activityId)
        { 
            auto message = Common::make_unique<Transport::Message>(DirectMessagingFailureBody(error));

            message->Headers.Add(Transport::ActorHeader(Transport::Actor::DirectMessagingAgent));
            message->Headers.Add(Transport::ActionHeader(DirectMessagingFailureAction));

            if (activityId.Guid != Common::Guid::Empty())
            {
                message->Headers.Add(Transport::FabricActivityHeader(activityId));
            }

            return message;
        }

        static void WrapServiceRequest(__inout Transport::Message &, SystemServiceLocation const &);
        static void WrapServiceRequest(__inout Client::ClientServerRequestMessage &, SystemServiceLocation const &);

        static Common::ErrorCode UnwrapFromTransport(__inout Transport::Message &);

        static Common::ErrorCode UnwrapServiceReply(__inout Transport::Message &);
        static Common::ErrorCode UnwrapServiceReply(__inout Client::ClientServerReplyMessage &);
    };
}
