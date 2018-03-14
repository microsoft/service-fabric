// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairTaskHealthCheckHelper
            : public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        private:
            DENY_COPY(RepairTaskHealthCheckHelper);            

        public:
            static RepairTaskHealthCheckState::Enum ComputeHealthCheckState(
                __in Store::ReplicaActivityId const & replicaActivityId,
                __in std::wstring const & taskId,
                __in Common::DateTime const & lastErrorAt,
                __in Common::DateTime const & lastHealthyAt,
                __in Common::DateTime const & healthCheckStart,
                __in Common::DateTime const & windowStart,
                __in Common::TimeSpan const & timeout);
        };
    }
}
