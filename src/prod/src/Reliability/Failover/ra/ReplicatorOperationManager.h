// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReplicatorOperationManager : public FailoverUnitProxyOperationManagerBase
        {
            DENY_COPY(ReplicatorOperationManager);

        public:
            ReplicatorOperationManager(FailoverUnitProxy & owner);
           
            void CancelOrMarkOperationForCancel(Common::ApiMonitoring::ApiName::Enum operation);
            void RemoveOperationForCancel(Common::ApiMonitoring::ApiName::Enum operation);
            
            FABRIC_QUERY_REPLICATOR_OPERATION_NAME GetNameForQuery() const;
        private:
            void GetApiCallDescriptionProperties(
                Common::ApiMonitoring::ApiName::Enum operation,
                __out bool & isSlowHealthReportRequired,
                __out bool & traceServiceType) const override;

            static FABRIC_QUERY_REPLICATOR_OPERATION_NAME GetNameForQuery(std::vector<Common::ApiMonitoring::ApiName::Enum> const & operations);

            static Common::Global<CompatibleOperationTable> CompatibilityTable;
        };
    }
}
