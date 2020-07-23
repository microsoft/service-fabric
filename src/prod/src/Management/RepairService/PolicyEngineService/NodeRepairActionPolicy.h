// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace RepairPolicyEngine
{
    class NodeRepairActionPolicy
    {
    public:
        NodeRepairActionPolicy(const std::wstring repairName, const std::wstring repairPolicySection);

        Common::ErrorCode Load(Common::ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr);
		

        /* After repair, Node needs to stay in AggregatedHealthState:Error during this interval to claim it in Failing state */
        const Common::TimeSpan& get_ProbationToFailingAfterRepairDelay() const
        {
            return probationToFailingAfterRepairDelay_;
        }

        /* After repair, Node needs to stay in AggregatedHealthState:Ok during this interval to claim it in Healthy state. */
        const Common::TimeSpan& get_ProbationToHealthyAfterRepairDelay() const
        {
            return probationToHealthyAfterRepairDelay_;
        }

        /* Define how many repair taks of this kind be scheduled at onetime */
        uint get_MaxOutstandingRepairTasks() const
        {
            return maxOutstandingRepairTasks_;
        }

        /* Minimum time of error state before this repair is applied. */
        const Common::TimeSpan& get_PolicyActionTime() const
        {
            return policyActionTime_;
        }

        /* Name of the repair. */
        const std::wstring& get_RepairName() const
        {
            return repairName_;
        }

        /* Optional. This is to control whether this node repair action is enabled */
        bool get_IsEnabled() const
        {
            return isEnabled_;
        }
        
     private:
        void TraceConfigurationValues();

        Common::TimeSpan probationToFailingAfterRepairDelay_;
        Common::TimeSpan probationToHealthyAfterRepairDelay_;
        Common::TimeSpan policyActionTime_;
        uint maxOutstandingRepairTasks_;
        bool isEnabled_;

        /* The name of this repair action policy section in configuration file */
        std::wstring repairPolicyNameSection_;

        /* The name of this repair action policy in configuration file */
        std::wstring repairName_;

        /* Constants used in parameter names in Settings.xml */
        static const std::wstring ProbationToFailingWaitDurationPostRepairInSecondsName;
        static const std::wstring ProbationToHealthyWaitDurationPostRepairInSecondsName;
        static const std::wstring MaxOutstandingRepairTasksName;
        static const std::wstring IsEnabledName;
        static const std::wstring PolicyActionTimeInSecondsName;
    };
}
