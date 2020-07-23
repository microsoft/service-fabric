// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class NodeRepairPolicyConfiguration
    {
    public:
	    NodeRepairPolicyConfiguration();

		Common::ErrorCode Load(Common::ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr);

		Common::ErrorCode LoadActionList(Common::ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr);

        void TraceConfigurationValues();

		/* Node needs to stay in AggregatedHealthState:Ok during this interval to claim it in Healthy state. */
        const Common::TimeSpan& get_ProbationToHealthyDelay() const
		{
			return probationToHealthyDelay_;
		}

        /* Node needs to stay in AggregatedHealthState:Error during this interval to claim it in Failing state and thus schedule a repair. */
        const Common::TimeSpan& get_ProbationToFailingDelay() const
		{
			return probationToFailingDelay_;
		}

		/* A health event is considered to be in health state Ok if within last MinimumHealthyDuration there was no Error� health state reported/updated for it.
	        This is to treat the health events that flip between ok and error health state. 
	        A flipping health event is considered to be in Error state if it flips ok->error->ok within the Now-MinimumErrorFreeTimeInSeconds� 
	        This is to control the reactivity of the RepairPolicyEngineService for such flipping events */
        const Common::TimeSpan& get_MinimumHealthyDuration() const
		{
			return minimumHealthyDuration_;
		}

		/* Optional. This is to control whether Node repair is enabled */
		bool get_IsEnabled() const
		{
			return isEnabled_;
		}
        
        /* The list of the NodeRepairActionPolicies*/
		const std::vector<NodeRepairActionPolicy>& get_NodeRepairActionPolicies() const
		{
			return NodeRepairActionMap_;
		}

	private:
        Common::TimeSpan probationToHealthyDelay_;
        Common::TimeSpan probationToFailingDelay_;
        Common::TimeSpan minimumHealthyDuration_;
		bool isEnabled_;

		/* The map of a repair action to its specific policy */
        std::vector<NodeRepairActionPolicy> NodeRepairActionMap_;

		/* Constants used in parameter names in Settings.xml */
		static const std::wstring ProbationToHealthyWaitDurationInSecondsName;
		static const std::wstring ProbationToFailingWaitDurationInSecondsName;
		static const std::wstring MinimumHealthyDurationInSecondsName;
		static const std::wstring IsEnabledName;
	};
}
