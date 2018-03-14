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
            template <class T>
            class SendMessageToFMAction : public StateMachineAction
            {
                DENY_COPY(SendMessageToFMAction);

            public:
                SendMessageToFMAction(
                    Reliability::FailoverManagerId const & target,
                    RSMessage const & message,
                    T && body) : 
                    message_(message),
                    body_(std::move(body)),
                    target_(target)
                {}

                void OnPerformAction(
                    std::wstring const & activityId, 
                    Infrastructure::EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent)
                {
                    reconfigurationAgent.FMTransportObj.SendEntityMessageToFM(
                        target_,
                        message_,
                        entity,
                        activityId,
                        std::move(body_));
                }

                void OnCancelAction(ReconfigurationAgent&)
                {
                }

            private:
                RSMessage const & message_;
                T body_;
                Reliability::FailoverManagerId target_;
            };

            template <class T>
            class SendMessageToRAAction : public StateMachineAction
            {
                DENY_COPY(SendMessageToRAAction);

            public:

                SendMessageToRAAction(
                    RSMessage const & message,
                    Federation::NodeInstance const & targetNode,
                    T && body)
                    : message_(message),
                    targetNode_(targetNode),
                    body_(std::move(body))
                {
                    TESTASSERT_IF(targetNode_ == ReconfigurationAgent::InvalidNode, "Cannot send to invalid node {0} {1}", message_.Action, body_);
                }

                void OnPerformAction(
                    std::wstring const & activityId, 
                    Infrastructure::EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent)
                {
                    RAEventSource::Events->SendMessageToRA(
                        entity->GetEntityIdForTrace(),
                        reconfigurationAgent.NodeInstance,
                        activityId,
                        message_.Action,
                        targetNode_,
                        Common::TraceCorrelatedEvent<T>(body_));

                    auto message = CreateTransportMessageForEntity(message_, body_, entity);

                    reconfigurationAgent.GenerationStateManagerObj.AddGenerationHeader(*message, body_.FailoverUnitDescription.IsForFMReplica);

                    reconfigurationAgent.Federation.SendToNode(std::move(message), targetNode_);
                }

                void OnCancelAction(ReconfigurationAgent &)
                {
                }

            private:

                RSMessage const & message_;
                Federation::NodeInstance targetNode_;
                T body_;
            };

            template <class T>
            class SendMessageToRAPBase : public StateMachineAction
            {
                DENY_COPY(SendMessageToRAPBase);

            protected:
                SendMessageToRAPBase(
                    RAMessage const & message,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    T && body) :
                    message_(message),
                    registration_(registration),
                    body_(std::move(body))
                {
                    ASSERT_IF(registration == nullptr, "ServiceTypeRegistration cannot be null");
                }

            public:
                void OnPerformAction(
                    std::wstring const & activityId, 
                    Infrastructure::EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & ra)
                {
                    RAEventSource::Events->SendMessageToRAP(
                        entity->GetEntityIdForTrace(),
                        ra.NodeInstance,
                        activityId,
                        message_.Action,
                        registration_->HostId,
                        static_cast<int>(registration_->HostProcessId),
                        Common::TraceCorrelatedEvent<T>(body_));

                    auto message = CreateTransportMessageForEntity(message_, body_, entity);
                    OnPerformActionInternal(std::move(message), ra);
                }

                void OnCancelAction(ReconfigurationAgent&) override
                {
                }

            private:
                virtual void OnPerformActionInternal(Transport::MessageUPtr && message, ReconfigurationAgent & ra) = 0;

            protected:
                RAMessage const & message_;
                Hosting2::ServiceTypeRegistrationSPtr registration_;
                T body_;
            };

            template <class T>
            class SendMessageToRAPAction : public SendMessageToRAPBase<T>
            {
                DENY_COPY(SendMessageToRAPAction);

            public:
                SendMessageToRAPAction(
                    RAMessage const & message,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    T && body) :
                    SendMessageToRAPBase<T>(message, registration, std::move(body))
                {
                }

            private:
                void OnPerformActionInternal(Transport::MessageUPtr && message, ReconfigurationAgent & ra) override
                {
                    ra.RAPTransport.SendOneWay(
                        this->registration_->HostId,
                        std::move(message),
                        ra.Config.RAPMessageRetryInterval);
                }
            };

            template <class T>
            class SendRequestReplyToRAPAction : public SendMessageToRAPBase<T>
            {
                DENY_COPY(SendRequestReplyToRAPAction);

            public:
                SendRequestReplyToRAPAction(
                    RAMessage const & message,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    T && body,
                    Common::AsyncCallback const & callback) :
                    SendMessageToRAPBase<T>(message, registration, std::move(body)),
                    callback_(callback)
                {
                }

            private:
                void OnPerformActionInternal(Transport::MessageUPtr && message, ReconfigurationAgent & ra) override
                {
                    /*
                        Handle the calling of the callback on the appropriate thread
                    */
                    auto callback = std::move(callback_);
                    auto operation = ra.RAPTransport.BeginRequest(
                        std::move(message),
                        this->registration_->HostId,
                        ra.Config.RAPMessageRetryInterval,
                        [callback] (Common::AsyncOperationSPtr const & inner)
                        {
                            if (!inner->CompletedSynchronously)
                            {
                                callback(inner);
                            }
                        },
                        ra.Root.CreateAsyncOperationRoot());

                    if (operation->CompletedSynchronously)
                    {
                        callback(operation);
                    }
                }

            private:
                Common::AsyncCallback callback_;
            };
        }
    }
}
