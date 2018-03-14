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
            class TestBase
            {
            protected:
                TestBase() : 
                updateContextQueue_(Common::make_unique<Infrastructure::StateMachineActionQueue>())
                {
                    ConfigurationManager configManager;
                    configManager.InitializeAssertConfig(true);
                    configManager.InitializeTraceConfig(1, 2, 0);
                }

                virtual ~TestBase()
                {
                }

                virtual Infrastructure::StateUpdateContext CreateContext()
                {
                    updateContextQueue_ = Common::make_unique<Infrastructure::StateMachineActionQueue>();
                    updateContext_ = StubUpdateContext();
                    return Infrastructure::StateUpdateContext(*updateContextQueue_, updateContext_);
                }

                void VerifyInMemoryCommit()
                {
                    updateContext_.CreateResult().VerifyInMemoryCommit();
                }

                void VerifyNoCommit()
                {
                    updateContext_.CreateResult().VerifyNoOp();
                }

                std::unique_ptr<Infrastructure::StateMachineActionQueue> updateContextQueue_;
                StubUpdateContext updateContext_;
            };
        }
    }
}



