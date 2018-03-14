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
            class ScenarioTestHolder
            {
            public:
                ScenarioTestHolder()
                {
                    Init(UnitTestContext::Option::StubJobQueueManager);
                }

                ScenarioTestHolder(UnitTestContext::Option::Enum options)
                {
                    Init(options);
                }

                ScenarioTestHolder(UnitTestContext::Option options)
                {
                    Init(options);
                }

                ~ScenarioTestHolder()
                {
                    scenarioTest_->Cleanup();
                }

                __declspec(property(get = get_ScenarioTestObj)) ScenarioTest & ScenarioTestObj;
                ScenarioTest & get_ScenarioTestObj() { return *scenarioTest_; }

                ScenarioTest &  Recreate()
                {
                    scenarioTest_->Cleanup();
                    scenarioTest_ = ScenarioTest::Create(options_);
                    return *scenarioTest_;
                }

                static std::unique_ptr<ScenarioTestHolder> Create(UnitTestContext::Option option)
                {
                    return Common::make_unique<ScenarioTestHolder>(option);
                }

                static std::unique_ptr<ScenarioTestHolder> Create(int option)
                {
                    return Common::make_unique<ScenarioTestHolder>(static_cast<UnitTestContext::Option::Enum>(option));
                }

                static std::unique_ptr<ScenarioTestHolder> Create()
                {
                    return Common::make_unique<ScenarioTestHolder>(UnitTestContext::Option::StubJobQueueManager);
                }

                static std::unique_ptr<ScenarioTestHolder> Create(UnitTestContext::Option::Enum options)
                {
                    return Common::make_unique<ScenarioTestHolder>(options);
                }

            private:
                void Init(UnitTestContext::Option options)
                {
                    options_ = options;
                    scenarioTest_ = ScenarioTest::Create(options_);
                }

                UnitTestContext::Option options_;
                ScenarioTestUPtr scenarioTest_;
            };
        }
    }
}
