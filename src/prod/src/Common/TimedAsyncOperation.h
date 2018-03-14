// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TimedAsyncOperation : public AsyncOperation
    {
        DENY_COPY(TimedAsyncOperation);

        public:
            TimedAsyncOperation(TimeSpan const timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent);
            virtual ~TimedAsyncOperation(void);
            static ErrorCode End(AsyncOperationSPtr const & thisSPtr);

            __declspec(property(get=get_OriginalTimeout)) TimeSpan OriginalTimeout;
            TimeSpan get_OriginalTimeout() const { return timeout_; }

            __declspec(property(get=get_RemainingTime)) TimeSpan RemainingTime;
            TimeSpan get_RemainingTime() const;

            __declspec(property(get=get_ExpireTime)) StopwatchTime ExpireTime;
            StopwatchTime get_ExpireTime() const { return expireTime_; }

        protected:

            virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override;
            virtual void OnCompleted(void) override;

            virtual void OnTimeout(AsyncOperationSPtr const & thisSPtr);

            // Should be used by derived class if the timer is being started outside
            // its own OnStart() function. The derived class is responsible
            // for including this call in its checks for start/completion racing.
            //
            void StartTimerCallerHoldsLock(AsyncOperationSPtr const & thisSPtr);

        private:
            void InternalStartTimer(AsyncOperationSPtr const & thisSPtr);
            static void OnTimerCallback(AsyncOperationSPtr const & thisSPtr);

            TimeSpan timeout_;
            StopwatchTime expireTime_;
            TimerSPtr timer_;
    };
}
