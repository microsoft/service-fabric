// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class RepairPolicyEngineService
    {
    public:
        RepairPolicyEngineService();

        HRESULT Initialize(Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr, IFabricCodePackageActivationContext *activationContext, Common::ManualResetEvent* exitServiceHostEventPtr, FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId);
        void RunAsync();
        void Stop(); 
        bool IsPrimaryReplica;
        
    private:
        void Run();
        HRESULT RefreshNodesRepairInfo(RepairPolicyEngineConfiguration& repairConfiguration, 
                                       bool firstRun, 
                                       const RepairPolicyEnginePolicy& repairPolicyEnginePolicy);
        
        bool PublishNodesRepairInfo(Common::ComPointer<IFabricHealthClient> fabricHealthClient, 
                                    const RepairPolicyEngineConfiguration& repairConfiguration);
        
        void FindOrAddNodeRepairInfo(std::wstring nodeNameToFind, __out std::shared_ptr<NodeRepairInfo>& foundNode);
        
        bool ScheduleRepairs(RepairPolicyEngineConfiguration& repairConfiguration, 
                             Common::ComPointer<IFabricHealthClient> fabricHealthClient);
        
        HRESULT GetNodeList(RepairPolicyEngineConfiguration & repairConfiguration, 
                            __out IFabricGetNodeListResult** nodeListResult);
        
        void FindLastExecutedRepairTime();
        
        void ReportHealth(RepairPolicyEngineConfiguration& repairConfiguration);
        
        void SetHealthState(FABRIC_HEALTH_STATE healthState);

        HRESULT BeginCreateRepairRequest(RepairPolicyEngineConfiguration& repairConfiguration, 
                                         Common::ComPointer<IFabricHealthClient> fabricHealthClient,
                                         std::shared_ptr<RepairPromotionRule> action, 
                                         std::shared_ptr<NodeRepairInfo> nodeRepairInfo);
        
        HRESULT BeginCancelRepairRequest(RepairPolicyEngineConfiguration& repairConfiguration,
                                         Common::ComPointer<IFabricHealthClient> fabricHealthClient,
                                         std::shared_ptr<NodeRepairInfo> nodeRepairInfo);

        bool FindStatusOfOutstandingRepairsForAllNodes(RepairPolicyEngineConfiguration& repairConfiguration, 
                                                       const RepairPolicyEnginePolicy& repairPolicyEnginePolicy,
                                                       bool firstRun);

        Common::ComPointer<IFabricCodePackageActivationContext> _activationContextCPtr;
        Common::ComPointer<IFabricQueryClient> _fabricQueryClientCPtr;
        Common::ComPointer<IFabricHealthClient> _fabricHealthClientCPtr;
		Common::ComPointer<IFabricRepairManagementClient> _fabricRepairManagementClientCPtr;
        std::vector<std::shared_ptr<NodeRepairInfo>> _nodesRepairInfo;
        Common::DateTime _lastExecutedRepairAt;
        bool _isRunning;
        Common::ExclusiveLock _lock;
        Common::ManualResetEvent _runStarted;
        Common::ManualResetEvent _runCompleted;
        Common::ManualResetEvent _runShouldExit;
        Common::ManualResetEvent* _exitServiceHostEventPtr;
        FABRIC_PARTITION_ID _partitionId;
        FABRIC_REPLICA_ID _replicaId;
        FABRIC_HEALTH_STATE _healthStateToReport;
        std::wstring _healthMessageOnProblem;
        Common::StringWriter _healthMessageWriter;
        Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr_;

        static WCHAR RepairPolicyEngineName[];
    };
}
