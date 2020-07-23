// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class NodeRepair : public Serialization::FabricSerializable
    {
    public:
        NodeRepair();
        NodeRepair(std::wstring repairType, Common::DateTime scheduledAt);
        Common::DateTime ScheduledAt;
        std::wstring RepairType;

        FABRIC_FIELDS_02(ScheduledAt, RepairType);
    };

    DEFINE_USER_ARRAY_UTILITY(NodeRepair);

    class NodeRepairInfo : public Serialization::FabricSerializable
    {
    public:
        /* Constructors */
        static std::shared_ptr<NodeRepairInfo> CreateNodeRepairInfo(std::wstring nodeName);
        NodeRepairInfo(std::wstring nodeName);

        /* Method signatures */
        void ScheduleFastTrackRepair(std::shared_ptr<RepairPromotionRule> rule);
        int GetCompletedRepairsCount(std::wstring repairType, Common::TimeSpan maximumRepairWindow);
        bool HasPendingRepair();
        Common::DateTime GetLastRepairAt();
        HRESULT Refresh(const FABRIC_NODE_QUERY_RESULT_ITEM * const nodeToRefreshFrom, Common::ComPointer<IFabricHealthClient> fabricHealthClient, RepairPolicyEngineConfiguration& repairConfiguration, const RepairPolicyEnginePolicy& repairPolicyEnginePolicy);
        // This is to publish selected state to health store for alerting and reporting purposes, not for state replication
        bool PublishToHealthStore(Common::ComPointer<IFabricHealthClient> fabricHealthClient, const RepairPolicyEngineConfiguration& repairConfiguration);
        void TraceState();
        std::wstring RepairTypeToSchedule();
        void RecoverNodeRepairToSchedule(FABRIC_REPAIR_TASK& repairTask, const RepairPolicyEnginePolicy& repairPolicyEnginePolicy);
        // The state transition of the nodes are reported through this function
        HRESULT ReportState(RepairStateTransition currentState, const RepairPolicyEngineConfiguration& repairConfiguration, Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr);

        /* Getters and setters, implemented inline */
        void set_IsRequestInProgress(bool value) { isRequestInProgress_ = value; }
        bool get_IsRequestInProgress() { return isRequestInProgress_; }
        void set_IsCancelInProgress(bool value) { isCancelInProgress_ = value; }
        bool get_IsCancelInProgress() { return isCancelInProgress_; }
        // should schedule cancel only if no cancel is in progress
        void set_ShouldScheduleCancel(bool value) { shouldScheduleCancel_ = value && !isCancelInProgress_; }
        bool get_shouldScheduleCancel() { return shouldScheduleCancel_; }
        bool IsFastTrackRepairProvided() const { return !FastTrackRepair.empty(); }
        bool IsRepairNotStarted() const { return repairProgressState_ == RepairProgressState::NotStarted; }
        bool IsRepairCreated() const { return repairProgressState_ == RepairProgressState::CreatedRepair; }

        /* Public variables */
        std::wstring NodeName;
        FABRIC_HEALTH_STATE HealthState;
        Common::DateTime EligibleFromProbationToHealthyAt;
        Common::DateTime EligibleFromProbationToFailingAt;
        std::shared_ptr<RepairPromotionRule> NodeRepairToSchedule;
        Common::DateTime LastErrorAt;
        Common::DateTime LastHealthyAt;
        NodePolicyState NodePolicyState;
        std::wstring LastRepairTaskId;        
        // Precondition: At last position is the most recent repair;
        std::vector<NodeRepair> RepairHistory;
        std::vector<std::shared_ptr<RepairPromotionRule>> FastTrackRepair;

        /* Fields to serialize */
        FABRIC_FIELDS_08(NodePolicyState, 
                         EligibleFromProbationToHealthyAt, EligibleFromProbationToFailingAt,
                         LastErrorAt, LastHealthyAt, 
                         RepairHistory, LastRepairTaskId, repairProgressState_);

    private:
        bool ReportHealth(std::wstring propertyName, std::wstring description, FABRIC_HEALTH_STATE healthState, Common::ComPointer<IFabricHealthClient> fabricHealthClient);
        HRESULT GetNodeHealthResult(LPCWSTR nodeName, RepairPolicyEngineConfiguration repairConfiguration, Common::ComPointer<IFabricHealthClient> fabricHealthClientCPtr, __out IFabricNodeHealthResult** nodeHealthResult);
        void TruncateRepairHistory(int maxCount);
        void GetRepairHistoryDescription(std::wstring& repairHistoryDescription);

        // This variable should be set ONLY in ReportState()
        RepairProgressState repairProgressState_;

        bool isRecentlyAdded_;
        bool shouldScheduleCancel_;
        volatile bool isRequestInProgress_;
        bool isCancelInProgress_;
    };
}
