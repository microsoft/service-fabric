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
            class StateMachineTestBase
            {
            protected:
                StateMachineTestBase()
                {
                    holder_ = ScenarioTestHolder::Create(UnitTestContext::Option::StubJobQueueManager | UnitTestContext::Option::TestAssertEnabled);
                }

                explicit StateMachineTestBase(int option)
                {
                    holder_ = ScenarioTestHolder::Create(option);
                }

                virtual ~StateMachineTestBase()
                {
                    holder_.reset();
                }

                __declspec(property(get = get_Test)) ScenarioTest & Test;
                ScenarioTest & get_Test() { return holder_->ScenarioTestObj; }

                __declspec(property(get = get_RA)) ReconfigurationAgent & RA;
                ReconfigurationAgent & get_RA() { return Test.RA; }

                void Drain()
                {
                    Test.DrainJobQueues();
                }

                void Recreate()
                {
                    holder_->Recreate();
                }

            private:
                ScenarioTestHolderUPtr holder_;
            };
        }
    }
}



