// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {

        namespace ReliabilityUnitTest
        {
            class RequestReceiverContextState
            {
                DENY_COPY(RequestReceiverContextState);
            public:
                enum Enum
                {
                    Created = 0,
                    Completed = 1,
                    Rejected = 2,
                };

                RequestReceiverContextState(std::wstring const & tag);

                __declspec(property(get = get_State)) Enum State;
                Enum get_State() const { return state_; }

                __declspec(property(get = get_ReplyMessage)) Transport::Message & ReplyMessage;
                Transport::Message & get_ReplyMessage() { return *replyMessage_; }

                __declspec(property(get = get_RejectErrorCode)) Common::ErrorCode RejectErrorCode;
                Common::ErrorCode get_RejectErrorCode() const { return rejectError_; }

                __declspec(property(get = get_Tag)) std::wstring const & Tag;
                std::wstring const & get_Tag() const { return tag_; }

                void Complete(Transport::MessageUPtr && reply);
                void Reject(Common::ErrorCode const & err);

            private:
                Enum state_;
                std::wstring tag_;
                Transport::MessageUPtr replyMessage_;
                Common::ErrorCode rejectError_;
            };

            typedef std::shared_ptr<RequestReceiverContextState> RequestReceiverContextStateSPtr;

            // Stub that tracks all the messages that are sent
            // Useful for asserting what was sent
            class FederationWrapperStub : public Reliability::IFederationWrapper
            {
                DENY_COPY(FederationWrapperStub)

            public:
                typedef std::map<Federation::NodeId, std::vector<Transport::MessageUPtr>> OtherNodeMessagesMap;
                typedef std::map<Transport::MessageId, RequestReceiverContextStateSPtr> RequestReceiverMap;

                FederationWrapperStub(Federation::NodeInstance const & instance)
                    : instance_(instance),
                    instanceIdStr_(instance.ToString()),
                    validateRequestReplyContext_(true)
                {
                }

                virtual ~FederationWrapperStub() {}

                Federation::NodeInstance const& get_Instance() const { return instance_; }
                std::wstring const & get_InstanceIdStr() const { return instanceIdStr_; }

                bool get_IsShutdown() const { return false; }

                __declspec(property(get = get_ValidateRequestReplyContext, put=set_ValidateRequestReplyContext)) bool ValidateRequestReplyContext;
                bool get_ValidateRequestReplyContext() const { return validateRequestReplyContext_; }
                void set_ValidateRequestReplyContext(bool const& value) { validateRequestReplyContext_ = value; }

                void IncrementInstanceId()
                {
                    instance_.IncrementInstanceId();
                    instanceIdStr_ = instance_.ToString();
                }

                Common::AsyncOperationSPtr BeginRequestToFMM(
                    Transport::MessageUPtr && message,
                    Common::TimeSpan const retryInterval,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) const;

                Common::ErrorCode EndRequestToFMM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const;

                Common::AsyncOperationSPtr BeginRequestToFM(
                    Transport::MessageUPtr && message,
                    Common::TimeSpan const retryInterval,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);
                Common::ErrorCode EndRequestToFM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);


                __declspec(property(get = get_FmMessages)) std::vector<Transport::MessageUPtr> & FmMessages;
                std::vector<Transport::MessageUPtr> & get_FmMessages()  { return fmMessages_; }

                __declspec(property(get = get_FmmMessages)) std::vector<Transport::MessageUPtr> & FmmMessages;
                std::vector<Transport::MessageUPtr> & get_FmmMessages() { return fmmMessages_; }

                __declspec(property(get = get_OtherNodeMessages)) OtherNodeMessagesMap & OtherNodeMessages;
                OtherNodeMessagesMap & get_OtherNodeMessages() { return otherNodeMessages_; }

                __declspec(property(get = get_RequestReceiverState)) RequestReceiverMap & RequestReceiverState;
                RequestReceiverMap & get_RequestReceiverState() { return requestReceiverContexts_; }

                void Reset();
                void SendToFM(Transport::MessageUPtr && message);
                void SendToFMM(Transport::MessageUPtr && message);
                void SendToNode(Transport::MessageUPtr && message, Federation::NodeInstance const & target);

                void VerifyNoMessagesSentToFMAndFMM();
                void VerifyNoMessagesSentToFM();
                void VerifyNoMessagesSentToFMM();

                void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Transport::MessageUPtr && reply);
                void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Common::ErrorCode const & error);

                Federation::RequestReceiverContextUPtr CreateRequestReceiverContext(Transport::MessageId const & source, std::wstring const & tag);

            private:
                RequestReceiverContextState & GetRequestReceiver(Federation::RequestReceiverContext & context);

                std::vector<Transport::MessageUPtr> fmMessages_;
                std::vector<Transport::MessageUPtr> fmmMessages_;

                OtherNodeMessagesMap otherNodeMessages_;

                RequestReceiverMap requestReceiverContexts_;

                Federation::NodeInstance instance_;
                std::wstring instanceIdStr_;
                bool validateRequestReplyContext_;
            };
        }
    }
}
