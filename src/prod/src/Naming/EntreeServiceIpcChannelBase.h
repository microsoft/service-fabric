// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeServiceIpcChannelBase
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>
    {
        DENY_COPY(EntreeServiceIpcChannelBase);

    public:
        EntreeServiceIpcChannelBase(Common::ComponentRoot const & root);

    protected:

        typedef std::function<Common::AsyncOperationSPtr(
            Transport::MessageUPtr & message,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const & )> BeginFunction;

        typedef std::function<Common::ErrorCode(
            Common::AsyncOperationSPtr const &,
            _Out_ Transport::MessageUPtr & )> EndFunction;

        bool RegisterHandler(
            Transport::Actor::Enum actor, 
            BeginFunction const &beginFunction,
            EndFunction const &endFunction);

        bool UnregisterHandler(Transport::Actor::Enum actor);

        template<typename T>
        class ProcessIpcRequestAsyncOperation
            : public Common::TimedAsyncOperation
        {
        public:
            ProcessIpcRequestAsyncOperation(
                EntreeServiceIpcChannelBase &owner,
                Transport::MessageUPtr &&message,
                std::unique_ptr<T> &&receiverContext,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent)
                : Common::TimedAsyncOperation(timeout, callback, parent)
                , owner_(owner)
                , message_(std::move(message))
                , receiverContext_(std::move(receiverContext))
            {
            }

            __declspec (property(get=get_IpcReceiverContext)) T const & ReceiverContext;
            T const & get_IpcReceiverContext() const { return *receiverContext_; }

            Transport::MessageUPtr && TakeReply() { return std::move(reply_); }

        protected:
            void OnStart(Common::AsyncOperationSPtr const &thisSPtr)
            {
                {
                    Common::AcquireReadLock readLock(owner_.handlerMapLock_);
                    auto iter = owner_.handlerMap_.find(message_->Actor);
                    if (iter == owner_.handlerMap_.end())
                    {
                        WriteWarning(
                            TraceType,
                            "Unknown actor {0}",
                            message_->Actor);
                        TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
                        return;
                    }

                    handler_ = iter->second;
                }

                message_->Headers.Replace(TimeoutHeader(RemainingTime));
                auto operation = handler_.first(
                    message_,
                    RemainingTime,
                    [this](AsyncOperationSPtr const &operation)
                {
                    this->OnHandlerFunctionComplete(operation, false);
                },
                    thisSPtr);

                OnHandlerFunctionComplete(operation, true);
            }

        private:
            void OnHandlerFunctionComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
            {
                if (operation->CompletedSynchronously != expectedCompletedSynchronously)
                {
                    return;
                }

                auto error = handler_.second(operation, reply_);
                TryComplete(operation->Parent, error);
            }

            EntreeServiceIpcChannelBase &owner_;
            Transport::MessageUPtr message_;
            std::unique_ptr<T> receiverContext_;
            Transport::MessageUPtr reply_;
            std::pair<BeginFunction, EndFunction> handler_;
        };

        template <typename T>
        void OnProcessIpcRequestComplete(
            Common::AsyncOperationSPtr const &operation,
            bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto casted = AsyncOperation::End<ProcessIpcRequestAsyncOperation<T>>(operation);
            if (!casted->Error.IsSuccess())
            {
                OnIpcFailure(casted->Error, casted->ReceiverContext);
                return;        
            }

            SendIpcReply(casted->TakeReply(), casted->ReceiverContext);
        }

        //
        // Methods that can be overridden in the derived class for special processing
        //
        virtual void OnIpcFailure(Common::ErrorCode const &error, Transport::ReceiverContext const & receiverContext) = 0;
        void SendIpcReply(Transport::MessageUPtr &&reply, Transport::ReceiverContext const & receiverContext);
    private:

        Common::RwLock handlerMapLock_;
        typedef std::pair<BeginFunction, EndFunction> Handler;
        std::map<Transport::Actor::Enum, Handler> handlerMap_;
    };
}
