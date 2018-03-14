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
            class RetryTimer
            {
                DENY_COPY(RetryTimer);

            public:

                // The callback provides a unique activity id to track this as the first paramter
                // and the RA as the second parameter
                typedef std::function<void(std::wstring const &, ReconfigurationAgent&)> RetryTimerCallback;

                RetryTimer(
                    std::wstring const & id,
                    ReconfigurationAgent & component,
                    TimeSpanConfigEntry const & retryInterval,
                    std::wstring const & activityIdPrefix,
                    RetryTimerCallback const & callback);

                ~RetryTimer();

                void Set();

                bool TryCancel(int64 sequenceNumber);

                void Close();

                __declspec (property(get = get_SequenceNumber)) int64 SequenceNumber;
                int64 get_SequenceNumber()
                {
                    Common::AcquireReadLock grab(lock_);

                    return sequenceNumber_;
                }

                __declspec(property(get = get_IsSet)) bool IsSet;
                bool get_IsSet()
                {
                    Common::AcquireReadLock grab(lock_);
                    return isSet_;
                }

                Common::Timer* Test_GetRawTimer()
                {
                    return timer_.get();
                }

                Common::TimeSpan GetRetryInterval() const;

            private:

                void SetTimerUnderLock();

                void CancelTimerUnderLock(bool recreate);

                void CreateTimerUnderLock();

                bool IsClosedUnderLock()
                {
                    return timer_ == nullptr;
                }

                std::wstring const id_;

                TimeSpanConfigEntry const & retryInterval_;

                Common::RwLock lock_;

                ReconfigurationAgent & component_;

                bool isSet_;
                int64 sequenceNumber_;
                Common::TimerSPtr timer_;
                std::wstring activityIdPrefix_;
                RetryTimerCallback callback_;
            };
        }
    }
}

