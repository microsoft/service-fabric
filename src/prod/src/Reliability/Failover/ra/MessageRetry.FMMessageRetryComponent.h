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
            class FMMessageRetryComponent
            {
                DENY_COPY(FMMessageRetryComponent);

            public:
                struct Parameters
                {
                    Parameters() : 
                    Target(*Reliability::FailoverManagerId::Fmm),
                    EntitySetCollection(nullptr),
                    RA(nullptr),
                    MinimumIntervalBetweenWork(nullptr),
                    RetryInterval(nullptr),
                    Throttle(nullptr)
                    {
                    }

                    Reliability::FailoverManagerId Target;
                    Infrastructure::EntitySetCollection * EntitySetCollection;
                    std::wstring DisplayName;
                    ReconfigurationAgent * RA;
                    TimeSpanConfigEntry * MinimumIntervalBetweenWork;
                    TimeSpanConfigEntry * RetryInterval;
                    Infrastructure::IThrottle * Throttle;
                    Node::PendingReplicaUploadStateProcessor const * PendingReplicaUploadState;
                };

                FMMessageRetryComponent(
                    Parameters & parameters);

                FMMessagePendingEntityList & Test_GetPendingEntityList() { return pendingEntityList_; }

                void Request(
                    std::wstring const & activityId);

                void Close();

            private:
                class FMMessageRetryAsyncOperation : public Common::AsyncOperation
                {
                public:
                    FMMessageRetryAsyncOperation(
                        std::wstring const & activityId,
                        FMMessageRetryComponent & component,
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent);

                protected:
                    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

                private:
                    Diagnostics::FMMessageRetryPerformanceData perfData_;
                    FMMessageRetryComponent & component_;
                    FMMessagePendingEntityList::EnumerationResult enumerationResult_;
                    std::wstring activityId_;
                };

                void OnBgmrRetry(std::wstring const & activityId);

                FMMessagePendingEntityList pendingEntityList_;
                FMMessageSender messageSender_;
                Infrastructure::BackgroundWorkManagerWithRetry bgmr_;
                ReconfigurationAgent & ra_;
                Reliability::FailoverManagerId target_;
            };
        }
    }
}
