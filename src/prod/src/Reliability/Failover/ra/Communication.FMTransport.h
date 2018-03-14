// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            /*
                Helper class around IFederationWrapper
            */
            class FMTransport
            {
            public:
                FMTransport(IFederationWrapper * federation) :
                    federation_(federation)
                {
                    ASSERT_IF(federation == nullptr, "Federation cannot be null - invalid value");
                }

                template<typename T>
                void SendMessageToFM(
                    Reliability::FailoverManagerId const & target,
                    RSMessage const & messageType,
                    std::wstring const & activityId,
                    T const & body)
                {
                    Infrastructure::RAEventSource::Events->SendNonFTMessageToFM(
                        federation_->InstanceIdStr,
                        activityId,
                        messageType.Action,
                        target,
                        Common::TraceCorrelatedEvent<T>(body));

                    auto message = messageType.CreateMessage(body);

                    Send(target, std::move(message));
                }

                template<typename T>
                void SendEntityMessageToFM(
                    Reliability::FailoverManagerId const & target,
                    RSMessage const & messageType,
                    Infrastructure::EntityEntryBaseSPtr const & entity,
                    std::wstring const & activityId,
                    T const & body)
                {
                    Infrastructure::RAEventSource::Events->SendMessageToFM(
                        entity->GetEntityIdForTrace(),
                        federation_->Instance,
                        activityId,
                        messageType.Action,
                        Common::TraceCorrelatedEvent<T>(body));

                    auto message = CreateTransportMessageForEntity(messageType, body, entity);

                    Send(target, std::move(message));
                }

                void SendServiceTypeInfoMessage(
                    std::wstring const & activityId,
                    ServiceTypeNotificationRequestMessageBody const & body)
                {
                    SendMessageToFM(
                        *FailoverManagerId::Fm,
                        RSMessage::GetServiceTypeNotification(),
                        activityId,
                        body);
                }

                void SendApplicationUpgradeReplyMessage(
                    std::wstring const & activityId,
                    NodeUpgradeReplyMessageBody const & body)
                {
                    SendMessageToFM(
                        *FailoverManagerId::Fm,
                        RSMessage::GetNodeUpgradeReply(),
                        activityId,
                        body);
                }

                void SendFabricUpgradeReplyMessage(
                    std::wstring const & activityId,
                    NodeFabricUpgradeReplyMessageBody const & body)
                {
                    SendMessageToFM(
                        *FailoverManagerId::Fm,
                        RSMessage::GetNodeFabricUpgradeReply(),
                        activityId,
                        body);
                }

                void SendCancelApplicationUpgradeReplyMessage(
                    std::wstring const & activityId,
                    NodeUpgradeReplyMessageBody const & body)
                {
                    SendMessageToFM(
                        *FailoverManagerId::Fm,
                        RSMessage::GetCancelApplicationUpgradeReply(),
                        activityId,
                        body);
                }

                void SendCancelFabricUpgradeReplyMessage(
                    std::wstring const & activityId,
                    NodeFabricUpgradeReplyMessageBody const & body)
                {
                    SendMessageToFM(
                        *FailoverManagerId::Fm,
                        RSMessage::GetCancelFabricUpgradeReply(),
                        activityId,
                        body);
                }


            private:
                void Send(
                    Reliability::FailoverManagerId const & target,
                    Transport::MessageUPtr && message)
                {
                    if (target.IsFmm)
                    {
                        federation_->SendToFMM(std::move(message));
                    }
                    else
                    {
                        federation_->SendToFM(std::move(message));
                    }
                }

                IFederationWrapper * federation_;
            };
        }
    }
}


