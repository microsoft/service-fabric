// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class FMMessageStateHolder
            {
            public:
                FMMessageStateHolder() :
                localReplicaDeleted_(false),
                retryPolicy_(std::make_shared<Infrastructure::LinearRetryPolicy>(config_)),
                messageState_(&localReplicaDeleted_, FailoverManagerId(true), std::dynamic_pointer_cast<Infrastructure::IRetryPolicy>(retryPolicy_))
                {
                    config_.Test_SetValue(Common::TimeSpan::FromSeconds(5));
                }


                __declspec(property(get = get_FMMessageStateObj)) FMMessageState & FMMessageStateObj;
                FMMessageState & get_FMMessageStateObj() { return messageState_; }

                __declspec(property(get = get_LocalReplicaDeleted, put=set_LocalReplicaDeleted)) bool LocalReplicaDeleted;
                bool get_LocalReplicaDeleted() const { return localReplicaDeleted_; }
                void set_LocalReplicaDeleted(bool const& value) { localReplicaDeleted_ = value; }

                __declspec(property(get = get_RetryPolicy)) Infrastructure::LinearRetryPolicy & RetryPolicy;
                Infrastructure::LinearRetryPolicy & get_RetryPolicy() { return *retryPolicy_; }

                __declspec(property(get = get_RetryInterval)) TimeSpanConfigEntry & RetryInterval;
                TimeSpanConfigEntry & get_RetryInterval() { return config_; }

            private:
                TimeSpanConfigEntry config_;
                std::shared_ptr<Infrastructure::LinearRetryPolicy> retryPolicy_;
                bool localReplicaDeleted_;
                FMMessageState messageState_;
            };
        }
    }
}



