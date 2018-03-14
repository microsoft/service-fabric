// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template <typename TMessage, typename TBody>
            inline Transport::MessageUPtr CreateTransportMessageForEntity(
                TMessage const & rsMessageType,
                TBody const & body,
                EntityEntryBaseSPtr const & entity)
            {
                auto message = rsMessageType.CreateMessage(body);
                entity->AddEntityIdToMessage(*message);
                return message;
            }

            template<typename T>
            inline bool TryGetMessageBody(std::wstring const & nodeInstanceId, Transport::Message & message, T & body)
            {
                if (!message.GetBody(body))
                {
                    Infrastructure::RAEventSource::Events->InvalidMessageDropped(nodeInstanceId, message.MessageId, message.Action, message.Status);
                    return false;
                }

                return true;
            }
        }
    }
}
