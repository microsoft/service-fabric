// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncOperationState
    {
    public:
        AsyncOperationState();
        ~AsyncOperationState();

        __declspec(property(get=get_CompletedSynchronously)) bool CompletedSynchronously;
        __declspec(property(get=get_IsCompleted)) bool IsCompleted;
        __declspec(property(get=get_InternalIsCompleted)) bool InternalIsCompleted;
        __declspec(property(get=get_InternalIsCompletingOrCompleted)) bool InternalIsCompletingOrCompleted;
        __declspec(property(get=get_InternalIsCancelRequested)) bool InternalIsCancelRequested;

        bool get_IsCompleted() const throw()
        {
            return (this->ObserveState() & CompletedMask) != 0;
        }

        bool get_CompletedSynchronously() const throw()
        {
            return (this->ObserveState() & ObservedBeforeComplete) == 0;
        }

        bool get_InternalIsCompleted() const throw()
        {
            return (this->operationState_.load() & CompletedMask) != 0;
        }

        bool get_InternalIsCancelRequested() const throw()
        {
            return (this->operationState_.load() & CancelRequested) != 0;
        }

         bool get_InternalIsCompletingOrCompleted() const throw()
        {
            return (this->operationState_.load() & CompletingOrCompletedMask) != 0;
        }

        void TransitionStarted();
        bool TryTransitionCompleting();
        void TransitionCompleted();
        bool TryTransitionEnded();
        void TransitionEnded();
        bool TryMarkCancelRequested();

    private:
        LONG ObserveState() const;

        mutable Common::atomic_long operationState_;

        static const LONG Created                = 0x01;
        static const LONG Started                = 0x02;
        static const LONG Completing             = 0x04;
        static const LONG Completed              = 0x08;
        static const LONG Ended                  = 0x10;
        static const LONG ObservedBeforeComplete = 0x20;
        static const LONG CancelRequested        = 0x40;
        
        static const LONG LifecycleMask = Created | Started | Completing | Completed | Ended;
        static const LONG CompletedMask = Completed | Ended;
        static const LONG CompletingOrCompletedMask = Completing | Completed | Ended;
    };
}
