// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class FailoverConfigContainer
            {
                DENY_COPY(FailoverConfigContainer);

            public:
                FailoverConfigContainer()
                {
                    std::wstring failoverConfig(L"[Failover]");
                    failoverConfigStore_ = std::make_shared<Common::FileConfigStore>(failoverConfig, true);
                    failoverConfig_ = Common::make_unique<Reliability::FailoverConfig>(failoverConfigStore_);
                    failoverConfig_->IsTestMode = true;
                }

                __declspec(property(get = get_Config)) Reliability::FailoverConfig & Config;
                Reliability::FailoverConfig & get_Config() { return *failoverConfig_; }

                __declspec(property(get = get_Store)) Common::FileConfigStore & Store;
                Common::FileConfigStore & get_Store() { return *failoverConfigStore_; }

                FailoverConfigContainer(FailoverConfigContainer && other)
                    : failoverConfig_(std::move(other.failoverConfig_)),
                    failoverConfigStore_(std::move(other.failoverConfigStore_))
                {
                }

                FailoverConfigContainer& operator=(FailoverConfigContainer && other)
                {
                    if (this != &other)
                    {
                        failoverConfig_ = std::move(other.failoverConfig_);
                        failoverConfigStore_ = std::move(other.failoverConfigStore_);
                    }
                    return *this;
                }

            private:

                Reliability::FailoverConfigUPtr failoverConfig_;
                Common::FileConfigStoreSPtr failoverConfigStore_;
            };
        }
    }
}
