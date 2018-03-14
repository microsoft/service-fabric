// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ServiceOperationManager : public FailoverUnitProxyOperationManagerBase
        {
            DENY_COPY(ServiceOperationManager);

        public:
            ServiceOperationManager(bool isStateful, FailoverUnitProxy & owner);

            void GetQueryInformation(
                FABRIC_QUERY_SERVICE_OPERATION_NAME & operationNameOut,  
                Common::DateTime & startTimeOut) const;

        private:
            static FABRIC_QUERY_SERVICE_OPERATION_NAME GetPublicApiOperationName(Common::ApiMonitoring::ApiName::Enum operation);

            static Common::Global<CompatibleOperationTable> CompatibilityTable;
        };
    }
}
