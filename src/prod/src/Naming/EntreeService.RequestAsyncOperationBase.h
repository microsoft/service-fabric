// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::RequestAsyncOperationBase
        : public Common::TimedAsyncOperation
        , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
    {
    public:
        RequestAsyncOperationBase(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        RequestAsyncOperationBase(
            __in GatewayProperties & properties,
            Common::NamingUri const & name,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        ~RequestAsyncOperationBase();

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Transport::MessageUPtr & reply);

    protected:
        __declspec(property(get=get_Properties)) GatewayProperties & Properties;
        GatewayProperties & get_Properties() const { return properties_; }

        __declspec(property(get=get_ReceivedMessage)) Transport::MessageUPtr const & ReceivedMessage;
        Transport::MessageUPtr const & get_ReceivedMessage() const { return receivedMessage_; }

        __declspec(property(get=get_NamingUri, put=set_NamingUri)) Common::NamingUri & Name;
        Common::NamingUri const & get_NamingUri() const { return namingUri_; }
        void set_NamingUri(Common::NamingUri const & value) { namingUri_ = value; }

        __declspec(property(get=get_Reply, put=set_Reply)) Transport::MessageUPtr & Reply;
        Transport::MessageUPtr const & get_Reply() const { return reply_; }
        void set_Reply(Transport::MessageUPtr && value) { reply_ = std::move(value); }

        __declspec(property(get=get_IsRetrying)) bool IsRetrying;
        bool get_IsRetrying() const { return (retryCount_ > 0); }

        __declspec(property(get=get_RetryCount)) uint64 RetryCount;
        uint64 get_RetryCount() const { return retryCount_; }

        void SetReplyAndComplete(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr && reply,
            Common::ErrorCode const & error);

        virtual void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);

        virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        virtual void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) = 0;

        void OnCancel();

        void HandleRetryStart(Common::AsyncOperationSPtr const & thisSPtr);
        bool TryHandleRetryStart(Common::AsyncOperationSPtr const & thisSPtr);

        virtual bool CanConsiderSuccess();

        void RouteToNode(
            Transport::MessageUPtr && request,
            Federation::NodeId const & nodeId,
            uint64 instanceId,
            bool useExactRouting,
            Common::AsyncOperationSPtr const & thisSPtr);

        virtual void OnRouteToNodeSuccessful(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply);

        virtual void OnRouteToNodeFailedRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply,
            Common::ErrorCode const & error);

        virtual void OnRouteToNodeFailedNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error);

        virtual bool IsRetryable(Common::ErrorCode const & error);

    protected:
        virtual void OnCompleted() override;

        bool TryGetNameFromRequestBody();

        virtual void ProcessReply(Transport::MessageUPtr & reply)
        {
            // no-op by default
            // The reply is modified by EntreeService.ForwardToServiceOperation
            // to put the ServiceLocation in case the message is a ListReply
            UNREFERENCED_PARAMETER(reply);
        }

    private:
        void OnRouteToNodeRequestComplete(Common::AsyncOperationSPtr const & asyncOperation, bool expectedCompletedSynchronously);
        void CancelRetryTimer();

        GatewayProperties & properties_;
        Transport::MessageUPtr receivedMessage_;

        Common::NamingUri namingUri_;
        Transport::MessageUPtr reply_;

        Common::ExclusiveLock retryTimerLock_;
        Common::TimerSPtr retryTimer_;
        uint64 retryCount_;
    };
}
