// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncOperation;

    typedef std::shared_ptr<AsyncOperation> AsyncOperationSPtr;

    // AsyncCallback type represents a pointer to the function that is called when an AsyncOperation is completed.
    typedef std::function<void(AsyncOperationSPtr const &)> AsyncCallback;
    typedef std::function<void(AsyncOperationSPtr const &)> CompletionCallback;

    struct OperationContextBase
    {
        std::unique_ptr<OperationContextBase> Next;

    public:
        virtual ~OperationContextBase() {};

    protected:
        OperationContextBase()
            : Next(nullptr)
        {
        }
    };

    template <typename TContext>
    struct OperationContext : public OperationContextBase
    {
        TContext Context;

        OperationContext(TContext && context)
            : Context(std::move(context))
        {
        }
    };

    // ************************************************************************************************************
    // AsyncOperation:
    //  This is a base type that represents an asynchronous operation. This type is used by derivation
    //  by the asynchronous operation writers. It contains enough information to execute the operation
    //  asynchronously and return results to users upon completion.
    //
    // ************************************************************************************************************
    class AsyncOperation
        : public std::enable_shared_from_this<AsyncOperation>
    {
        DENY_COPY(AsyncOperation)

    public:
        virtual ~AsyncOperation(void);

        __declspec(property(get=get_AsyncOperationTraceId)) std::wstring const & AsyncOperationTraceId;

        // returns the ErrorCode from the result of AsyncOperation
        __declspec(property(get=get_Error)) ErrorCode const & Error;

        // returns the parent supplied to this AsyncOperation
        __declspec(property(get=get_State)) AsyncOperationSPtr const & Parent;

        __declspec(property(get=get_CompletedSynchronously)) bool CompletedSynchronously;
        __declspec(property(get=get_IsCompleted)) bool IsCompleted;
        __declspec(property(get=get_IsCancelRequested)) bool IsCancelRequested;
        __declspec(property(get=get_FailedSynchronously)) bool FailedSynchronously;

        inline std::wstring const & AsyncOperation::get_AsyncOperationTraceId() const
        {
            return traceId_;
        }

        inline ErrorCode const & AsyncOperation::get_Error() const
        {
            return error_;
        }

        inline AsyncOperationSPtr const & AsyncOperation::get_State() const
        {
            return parent_;
        }

        inline bool get_IsCompleted() const
        {
            return state_.IsCompleted;
        }

        inline bool get_CompletedSynchronously() const
        {
            return state_.CompletedSynchronously;
        }

        inline bool get_IsCancelRequested() const
        {
            return state_.InternalIsCancelRequested;
        }

        inline bool get_FailedSynchronously() const
        {
            return (this->CompletedSynchronously && !this->Error.IsSuccess());
        }

        // This method starts this AsyncOperation.
        void Start(AsyncOperationSPtr const & thisSPtr);

        // This method is called to indicate an intent to complete this AsyncOperation.
        // It returns true if the operation was not completed and returns false if the
        // operation was already completed or was in the process of completion.
        // Once TryStartComplete returns true FinishComplete must be called to supply the
        // error code and invoke completion callback.
        bool TryStartComplete();

        // This method finishes the completion of this operation that was started by
        // TryStartComplete method. This method must be called once and only after
        // TryStartComplete returns a success. This method sets the specified
        // ErrorCode to this operation and calls user specified completion callback.
        void FinishComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error = ErrorCode());
        void FinishComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode && error);

        // This method will try to complete this AsyncOperation with the specified error.
        // The AsyncOperation must be started. It returns true if the AsyncOperation was
        // completed successfully, otherwise false.
        bool TryComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error = ErrorCode());
        bool TryComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode && error);

        // Attempts to Cancel the current operation.  There is no guarantee that this
        // will succeed, as it may be called too late (as the operation is already completing).
        void Cancel(bool forceComplete = false);

        template<typename TContext>
        void PushOperationContext(std::unique_ptr<OperationContext<TContext>> && context)
        {
            context->Next = std::move(contexts_);
            contexts_ = std::move(context);
        }

        template<typename TContext>
        std::unique_ptr<OperationContext<TContext>> PopOperationContext()
        {
#if DBG && !defined(PLATFORM_UNIX)
            OperationContext<TContext>* result = dynamic_cast<OperationContext<TContext> *>(contexts_.release());
#else
            OperationContext<TContext>* result = static_cast<OperationContext<TContext> *>(contexts_.release());
#endif
            if (result)
            {
                contexts_ = std::move(result->Next);
            }

            return std::unique_ptr<OperationContext<TContext>>(result);
        }

        template<typename TAsyncOperation>
        static AsyncOperationSPtr CreateAndStart(void)
        {
            AsyncOperationSPtr asyncOperation = std::make_shared<TAsyncOperation>();
            asyncOperation->Start(asyncOperation);

            return asyncOperation;
        }

		template<typename TAsyncOperation, class ...Args>
		static AsyncOperationSPtr CreateAndStart(Args&& ...args)
		{
			AsyncOperationSPtr asyncOperation = std::make_shared<TAsyncOperation>(std::forward<Args>(args)...);
			asyncOperation->Start(asyncOperation);

			return asyncOperation;
		}

        template<typename TAsyncOperation>
        static TAsyncOperation* Get(AsyncOperationSPtr const & asyncOperation)
        {
#if DBG && !defined(PLATFORM_UNIX)
            auto tAsyncOperation = dynamic_cast<TAsyncOperation*>(asyncOperation.get());
#else
            auto tAsyncOperation = static_cast<TAsyncOperation*>(asyncOperation.get());
#endif

            ASSERT_IF(tAsyncOperation == nullptr, "AsyncOperation<TAsyncOperation>::Get(...) tAsyncOperation == nullptr");

            return tAsyncOperation;
        }

        static ErrorCode End(AsyncOperationSPtr const & asyncOperation)
        {
            asyncOperation->state_.TransitionEnded();
            return asyncOperation->Error;
        }

        template<typename TAsyncOperation>
        static TAsyncOperation* End(AsyncOperationSPtr const & asyncOperation)
        {
            auto tAsyncOperation = Get<TAsyncOperation>(asyncOperation);
            ASSERT_IF(asyncOperation->contexts_ != nullptr, "Context not empty when operation ended");
            asyncOperation->state_.TransitionEnded();
            return tAsyncOperation;
        }

        static void CancelIfNotNull(AsyncOperationSPtr const & asyncOperation)
        {
            if (asyncOperation != nullptr)
            {
                asyncOperation->Cancel();
            }
        }

    protected:
        AsyncOperation();
        AsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent, bool skipCompleteOnCancel = false);

        // Queries completion status without affecting CompletedSynchronously.
        __declspec(property(get=get_InternalIsCompleted)) bool InternalIsCompleted;
        inline bool get_InternalIsCompleted() const { return state_.InternalIsCompleted; }

        __declspec(property(get=get_Test_ChildrenCount)) size_t Test_ChildrenCount;
        inline size_t get_Test_ChildrenCount() const { return children_.size(); }

        // The derive type override this method to implement the logic of the asynchronous operation.
        // This method will be called once, immediately after the construction of the OperationContext
        virtual void OnStart(AsyncOperationSPtr const & thisSPtr) = 0;
        virtual void OnCancel();
        virtual void OnCompleted();

        bool AttachChild(AsyncOperationSPtr && child);
        bool AttachChild(AsyncOperationSPtr const & child);
        void DetachChild(AsyncOperationSPtr const & child);

    private:
        void FinishCompleteInternal(AsyncOperationSPtr const & thisSPtr);
        
        void Cleanup(AsyncOperationSPtr const & thisSPtr);

        std::wstring traceId_;
        bool skipCompleteOnCancel_;
        AsyncOperationState state_;
        AsyncCallback callback_;
        AsyncOperationSPtr parent_;
        ErrorCode error_;
        Common::ExclusiveLock childLock_;
        std::list<std::weak_ptr<AsyncOperation>> children_;
        std::unique_ptr<OperationContextBase> contexts_;
    };

    template <typename TAsyncOperationRoot>
    class AsyncOperationRoot : public AsyncOperation
    {
        DENY_COPY(AsyncOperationRoot)

    public:
        AsyncOperationRoot(TAsyncOperationRoot const & value)
            : AsyncOperation(),
              value_(value)
        {
            Error.IsSuccess(); // observe the error
        }

        AsyncOperationRoot(TAsyncOperationRoot && value)
            : AsyncOperation(),
              value_(std::move(value))
        {
            Error.IsSuccess(); // observe the error
        }

        ~AsyncOperationRoot(void) { }

        static AsyncOperationSPtr Create(TAsyncOperationRoot const & root)
        {
            return std::static_pointer_cast<AsyncOperation>(std::make_shared<AsyncOperationRoot<TAsyncOperationRoot>>(root));
        }

        static AsyncOperationSPtr Create(TAsyncOperationRoot && root)
        {
            return std::static_pointer_cast<AsyncOperation>(std::make_shared<AsyncOperationRoot<TAsyncOperationRoot>>(std::move(root)));
        }

        static TAsyncOperationRoot & Get(AsyncOperationSPtr const & parent)
        {
            auto rootAsyncOperation = static_cast<AsyncOperationRoot<TAsyncOperationRoot>*>(parent.get());
            ASSERT_IF(rootAsyncOperation == nullptr, "AsyncOperation<TAsyncOperationRoot>::GetRoot(...) rootAsyncOperation == nullptr");

            return rootAsyncOperation->value_;
        }

    protected:
        void OnStart(AsyncOperationSPtr const &) { };

    private:
        TAsyncOperationRoot value_;
    };

    template <typename TContext>
    class OperationContextCallback
    {
    public:
        OperationContextCallback(AsyncCallback const & callback, TContext && context)
            : callback_(callback),
              context_(make_unique<OperationContext<TContext>>(std::move(context)))
        {
        }

        __declspec(property(get=get_Context)) TContext & Context;
        TContext & get_Context() { return context_->Context; }

        void operator()(AsyncOperationSPtr const & operation)
        {
            operation->PushOperationContext(context_.TakeUPtr());

            if (callback_)
            {
                callback_(operation);
            }
        }

    private:
        AsyncCallback callback_;
        MoveUPtr<OperationContext<TContext>> context_;
    };
}
