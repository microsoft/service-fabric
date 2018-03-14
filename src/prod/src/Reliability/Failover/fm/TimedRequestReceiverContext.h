// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class TimedRequestReceiverContext
        {
            DENY_COPY(TimedRequestReceiverContext)

        public:

            TimedRequestReceiverContext(
                FailoverManager & fm,
                Federation::RequestReceiverContextUPtr && context,
                std::wstring const& action);

            __declspec (property(get = get_Context)) Federation::RequestReceiverContextUPtr & Context;
            Federation::RequestReceiverContextUPtr & get_Context() { return context_; }

            void Reject(Common::ErrorCode const& error, Common::ActivityId const& activityId = Common::ActivityId::Empty);

            void Reply(Transport::MessageUPtr && reply);

        private:

            void TraceIfSlow() const;

            FailoverManager & fm_;
            Federation::RequestReceiverContextUPtr context_;
            std::wstring action_;
            Common::Stopwatch stopwatch_;
        };
    }
}
