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
            class StubUpdateContext : public Infrastructure::UpdateContext
            {
            public:
                StubUpdateContext() : enableUpdateCalled_(false), enableInMemoryUpdateCalled_(false)
                {
                }

                void EnableUpdate()
                {
                    enableUpdateCalled_ = true;
                    enableInMemoryUpdateCalled_ = false;
                }
                
                void EnableInMemoryUpdate()
                {
                    if (enableUpdateCalled_)
                    {
                        return;
                    }

                    enableInMemoryUpdateCalled_ = true;
                }
                
                CommitTypeDescriptionUtility CreateResult()
                {
                    if (!enableUpdateCalled_ && !enableInMemoryUpdateCalled_)
                    {
                        return CommitTypeDescriptionUtility();
                    }
                    else
                    {
                        Infrastructure::CommitTypeDescription d;
                        d.StoreOperationType = Storage::Api::OperationType::Update;
                        d.IsInMemoryOperation = enableInMemoryUpdateCalled_;
                        return CommitTypeDescriptionUtility(d);
                    }
                }

            private:
                bool enableUpdateCalled_;
                bool enableInMemoryUpdateCalled_;
            };
        }
    }
}
