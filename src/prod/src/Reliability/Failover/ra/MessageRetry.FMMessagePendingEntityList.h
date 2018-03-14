// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace MessageRetry
        {
            class FMMessagePendingEntityList
            {
                DENY_COPY(FMMessagePendingEntityList);

            public:
                struct EnumerationResult
                {
                    EnumerationResult() : RetryType(Infrastructure::BackgroundWorkRetryType::None)
                    {
                    }

                    __declspec(property(get = get_Count)) size_t Count;
                    size_t get_Count() const { return Messages.size(); }

                    Infrastructure::BackgroundWorkRetryType::Enum RetryType;

                    std::vector<Communication::FMMessageDescription> Messages;
                };

                FMMessagePendingEntityList(
                    Reliability::FailoverManagerId const & fm,
                    std::wstring const & displayName,
                    Infrastructure::EntitySetCollection & setCollection,
                    Infrastructure::IThrottle & throttle,
                    ReconfigurationAgent & ra);

                void Test_UpdateThrottle(Infrastructure::IThrottle * throttle)
                {
                    throttle_ = throttle;
                }

                EnumerationResult Enumerate(
                    __inout Diagnostics::FMMessageRetryPerformanceData & perfData);

                Common::AsyncOperationSPtr BeginUpdateEntitiesAfterSend(
                    std::wstring const & activityId,
                    __inout Diagnostics::FMMessageRetryPerformanceData & perfData, // guaranteed to be kept alive by parent
                    EnumerationResult const & enumerationResult, // guaranteed to be kept alive by parent
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);

                Common::ErrorCode EndUpdateEntitiesAfterSend(
                    Common::AsyncOperationSPtr const & op);

            private:
                class AsyncOperationImpl : public Common::AsyncOperation
                {
                public:
                    AsyncOperationImpl(
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) :
                        Common::AsyncOperation(callback, parent)
                    {
                    }

                protected:
                    void OnStart(Common::AsyncOperationSPtr const &) override
                    {
                    }
                };

                __declspec(property(get = get_Clock)) Infrastructure::IClock const & Clock;
                Infrastructure::IClock const & get_Clock() const;

                EnumerationResult CreateResult(
                    Infrastructure::EntityEntryBaseList && entities);

                std::vector<Communication::FMMessageDescription> CreateMessages(
                    int throttle,
                    Infrastructure::EntityEntryBaseList && entities);

                Infrastructure::BackgroundWorkRetryType::Enum ComputeRetryType(
                    int throttle,
                    size_t pendingItemCount) const;


                bool TryComposeFMMessage(
                    Common::StopwatchTime now,
                    Infrastructure::EntityEntryBaseSPtr const & entity,
                    __out Communication::FMMessageData & message);

                Infrastructure::EntitySet entitySet_;
                Infrastructure::IThrottle * throttle_;
                ReconfigurationAgent & ra_;
            };
        }
    }
}



