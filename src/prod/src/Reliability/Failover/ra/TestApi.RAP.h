// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliabilityTestApi
{
    namespace ReconfigurationAgentComponentTestApi
    {
        class ReconfigurationAgentProxyTestHelper
        {
            DENY_COPY(ReconfigurationAgentProxyTestHelper);
        public:
            ReconfigurationAgentProxyTestHelper(Reliability::ReconfigurationAgentComponent::IReconfigurationAgentProxy & rap);

            std::vector<Reliability::FailoverUnitId> GetItemsInLFUPM();

        private:
            Reliability::ReconfigurationAgentComponent::ReconfigurationAgentProxy & rap_;
        };
    }
}
