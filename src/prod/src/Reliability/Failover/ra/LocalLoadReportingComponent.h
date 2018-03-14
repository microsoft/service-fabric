// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // LocalLoadReportingComponent; used by RAP to manage load reporting
        class LocalLoadReportingComponent: 
            public Common::RootedObject
        {
            DENY_COPY(LocalLoadReportingComponent);

        public:
            LocalLoadReportingComponent(
                Common::ComponentRoot const & root,
                Transport::IpcClient & ipcClient);
            ~LocalLoadReportingComponent();

            void Open(ReconfigurationAgentProxyId const & rapId);
            void Close();
            void SetTimer();
            void UpdateTimer();

            void ReportLoadCallback();
            
            void AddLoad(
                FailoverUnitId const& fuId, 
                std::wstring && serviceName, 
                bool isStateful, 
                Reliability::ReplicaRole::Enum replicaRole, 
                std::vector<LoadBalancingComponent::LoadMetric> && loadMetrics,
                Federation::NodeId const& nodeId);

            void RemoveFailoverUnit(FailoverUnitId const & fuId);

        private:
            static bool IsValidLoad(
                bool isStateful, 
                Reliability::ReplicaRole::Enum replicaRole, 
                std::vector<LoadBalancingComponent::LoadMetric> const & loadMetrics);

            std::map<FailoverUnitId, LoadBalancingComponent::LoadOrMoveCostDescription> failoverUnitLoadMap_;
            
            bool open_;
            Common::TimerSPtr reportTimerSPtr_;
            Transport::IpcClient & ipcClient_;

            ReconfigurationAgentProxyId id_;

            Common::RwLock lock_;
        };
    }
}
