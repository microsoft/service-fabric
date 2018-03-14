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
            class EntityExecutionContextTestUtility
            {
                DENY_COPY(EntityExecutionContextTestUtility);

            public:
                typedef std::function<void(Infrastructure::EntityExecutionContext&)> EntityProcessor;              

                EntityExecutionContextTestUtility(ScenarioTest & test);

                CommitTypeDescriptionUtility ExecuteEntityProcessor(std::wstring const & ftShortName, EntityProcessor const & processor);

            private:
                ScenarioTest & test_;
            };
        }
    }
}



