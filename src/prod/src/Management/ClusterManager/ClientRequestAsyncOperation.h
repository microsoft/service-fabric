// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define BEGIN_DECLARE_CLIENT_ASYNCOPERATION( Operation ) \
namespace Management \
{ \
    namespace ClusterManager \
    { \
        class Operation##AsyncOperation : public ClientRequestAsyncOperation \
        { \
        public: \
            Operation##AsyncOperation( \
                __in ClusterManagerReplica & owner, \
                Transport::MessageUPtr && request, \
                Federation::RequestReceiverContextUPtr && requestContext, \
                Common::TimeSpan const timeout, \
                Common::AsyncCallback const & callback, \
                Common::AsyncOperationSPtr const & parent) \
                : ClientRequestAsyncOperation( \
                    owner, \
                    move(request), \
                    move(requestContext), \
                    timeout, \
                    callback, \
                    parent) \
            { \
            } \
        protected: \
            Common::AsyncOperationSPtr BeginAcceptRequest( \
                ClientRequestSPtr &&, \
                Common::TimeSpan const,  \
                Common::AsyncCallback const &,  \
                Common::AsyncOperationSPtr const &) override; \
            Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &) override; \

#define END_DECLARE_CLIENT_ASYNCOPERATION \
        }; \
    } \
} \

#define DECLARE_CLIENT_ASYNCOPERATION( Operation ) \
    BEGIN_DECLARE_CLIENT_ASYNCOPERATION( Operation ) \
    END_DECLARE_CLIENT_ASYNCOPERATION \

namespace Management
{
    namespace ClusterManager
    {
        class ClusterManagerReplica;
        class ClientRequestAsyncOperation
            : public Common::TimedAsyncOperation
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
        public:
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>::TraceId;
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>::get_TraceId;
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>::ActivityId;
            using Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>::get_ActivityId;

            ClientRequestAsyncOperation(
                __in ClusterManagerReplica &,
                Transport::MessageUPtr &&,
                Federation::RequestReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            ClientRequestAsyncOperation(
                __in ClusterManagerReplica &,
                Common::ErrorCode const &,
                Transport::MessageUPtr &&,
                Federation::RequestReceiverContextUPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ClientRequestAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply,
                __out Federation::RequestReceiverContextUPtr & requestContext);

            __declspec(property(get=get_Timeout)) Common::TimeSpan Timeout;
            __declspec(property(get=get_RequestInstance)) __int64 RequestInstance;
            __declspec(property(get=get_RequestMessageId)) Transport::MessageId const & RequestMessageId;

            Common::TimeSpan get_Timeout() const { return this->OriginalTimeout; }
            __int64 get_RequestInstance() const { return requestInstance_; }
            Transport::MessageId get_RequestMessageId() const { return request_->MessageId; }

            void SetReply(Transport::MessageUPtr &&);

            void CompleteRequest(Common::ErrorCode const &);

            bool IsLocalRetry();

            static bool IsRetryable(Common::ErrorCode const &);

        protected:

            __declspec(property(get=get_Replica)) ClusterManagerReplica & Replica;
            __declspec(property(get=get_Request)) Transport::Message & Request;
            __declspec(property(get=get_RequestContext)) Federation::RequestReceiverContext const & RequestContext;

            ClusterManagerReplica & get_Replica() const { return owner_; }
            Transport::Message & get_Request() const { return *request_; }
            Federation::RequestReceiverContext const & get_RequestContext() const { return *requestContext_; }

            void OnStart(Common::AsyncOperationSPtr const &) override;
            void OnTimeout(Common::AsyncOperationSPtr const &) override;
            void OnCancel() override;
            void OnCompleted() override;

            virtual Common::AsyncOperationSPtr BeginAcceptRequest(
                std::shared_ptr<ClientRequestAsyncOperation> &&,
                Common::TimeSpan const, 
                Common::AsyncCallback const &, 
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &);

        private:
            void HandleRequest(Common::AsyncOperationSPtr const &);
            void OnAcceptRequestComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void FinishRequest(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);
            void CancelRetryTimer();

            static Common::ErrorCode ToGatewayError(Common::ErrorCode const &);

            ClusterManagerReplica & owner_;
            Common::ErrorCodeValue::Enum error_;
            Transport::MessageUPtr request_;
            Transport::MessageUPtr reply_;
            Federation::RequestReceiverContextUPtr requestContext_;
            __int64 requestInstance_;
            Common::TimerSPtr retryTimer_;
            Common::ExclusiveLock timerLock_;
            bool isLocalRetry_;
        };

        typedef std::shared_ptr<ClientRequestAsyncOperation> ClientRequestSPtr;
    }
}
