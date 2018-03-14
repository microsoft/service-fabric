// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<class Tin, class Tout>
    class ParallelAsyncOperationBase
        : public AsyncOperation
    {
        DENY_COPY(ParallelAsyncOperationBase)

    public:
        ParallelAsyncOperationBase(
            std::size_t const size,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) :
            AsyncOperation(callback, parent),
            size_(size)
        {
        }

        virtual ~ParallelAsyncOperationBase()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation, __out std::vector<Tout> & output)
        {
            auto thisSPtr = AsyncOperation::End<ParallelAsyncOperationBase>(operation);
            output = move(thisSPtr->output_);
            return thisSPtr->Error;
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto thisSPtr = AsyncOperation::End<ParallelAsyncOperationBase>(operation);
            return thisSPtr->Error;
        }

    protected:
        virtual void OnStartInternal(AsyncOperationSPtr const & thisSPtr) = 0;
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            if(size_ == 0)
            {
                TryComplete(thisSPtr);
                return;
            }

            count_.store(size_);

            OnStartInternal(thisSPtr);
        }

        virtual ErrorCode OnEndOperation(AsyncOperationSPtr const & operation, Tin const & input, __out Tout & output)=0;

        void OperationCompleted(AsyncOperationSPtr const & operation, Tin const & input, bool expectedCompletedSynchronously)
        {
            if(operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            Tout result;
            ErrorCode er = OnEndOperation(operation, input, result);

            if(er.IsSuccess())
            {
                // OperationCompleted is called concurrently
                AcquireWriteLock acquiredLock(lock_);
                output_.push_back(result);
            }
            else
            {
                AcquireWriteLock acquiredLock(lock_);
                lastError_.Overwrite(er);
            }

            uint64 currentCount = --count_;
            if(currentCount != 0){
                return;
            }

            TryComplete(operation->Parent, lastError_);
        }

        std::size_t const size_;

    private:
        atomic_uint64 count_;
        std::vector<Tout> output_;
        RwLock lock_;
        ErrorCode lastError_;
    };
}
