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
            // Class that encapsulates a background work manager and a retry timer
            // For operations that happen in the background and need retris
            // Example: Sending of messages/Handling service type requests etc
            // Callers can request work anytime and these requests will be honoured as per the BGM itself
            // The callback function will be executed on the job queue
            // When the callback completes (sync or async) it should tell the bgm whether a retry is needed or not
            // The BGM will then arm the retry timer as required
            class BackgroundWorkManagerWithRetry
            {
                DENY_COPY(BackgroundWorkManagerWithRetry);

            public:

                // The callback
                // It is passed in activity id, RA and this object
                typedef std::function<void(std::wstring const &, ReconfigurationAgent&, BackgroundWorkManagerWithRetry&)> WorkFunctionPointer;

                BackgroundWorkManagerWithRetry(
                    std::wstring const & id,
                    std::wstring const & timerActivityIdPrefix,
                    WorkFunctionPointer const & workFunction,
                    TimeSpanConfigEntry const & minIntervalBetweenWork,
                    TimeSpanConfigEntry const & retryInterval,
                    ReconfigurationAgent & ra);

                __declspec(property(get = get_RetryTimer)) RetryTimer & RetryTimerObj;
                RetryTimer & get_RetryTimer() { return retryTimer_; }

                __declspec(property(get = get_BackgroundWorkManagerObj)) BackgroundWorkManager & BackgroundWorkManagerObj;
                BackgroundWorkManager & get_BackgroundWorkManagerObj()  { return bgm_; }

                // Request the activity id
                void Request(std::wstring const & activityId);

                void OnWorkComplete(bool isRetryRequired);

                void OnWorkComplete(BackgroundWorkRetryType::Enum retryType);

                void Close();

                void SetTimer();

                // public to get around the inability to call this from a member init list in a constructor
                void OnBGMCallback_Internal(std::wstring const & activityId, ReconfigurationAgent & component);

                void OnRetryTimerCallback_Internal(std::wstring const & activityId);

            private:
                std::wstring lastActivityId_;
                BackgroundWorkManager bgm_;
                RetryTimer retryTimer_;
                int64 sequenceNumberWhenCallbackStarted_;
                WorkFunctionPointer workFunction_;
            };
        }
    }
}
