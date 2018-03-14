// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ProxyActionsList::ConcurrentExecutionCompatibilityLookupTable {
            DENY_COPY(ConcurrentExecutionCompatibilityLookupTable);

        public:
            ConcurrentExecutionCompatibilityLookupTable();

            bool CanExecuteConcurrently(ProxyActionsListTypes::Enum const runningAction, ProxyActionsListTypes::Enum const actionToRun) const;

        private:
            bool lookupTable_[ProxyActionsListTypes::ProxyActionsListTypesCount][ProxyActionsListTypes::ProxyActionsListTypesCount]; 

            void MakeCompatibleWithEverythingExceptLifecycleAndSelf(ProxyActionsListTypes::Enum action);
        };
    }
}
