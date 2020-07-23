// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    // Can this be a singleton? 
    class RepairPolicyEngineConfiguration
    {
    public:
        RepairPolicyEngineConfiguration();

        Common::ErrorCode Load(Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr);

        void TraceConfigurationValues();

        /* PE needs to remember the last scheduled repair to not schedule another repair for the same node twice */
        uint MaxNumberOfRepairsToRemember;

        /* RepairPolicyEngineService is not allowed to schedule a repair more often than this interval.
        	This is to prevent the RepairPolicyEngineService from scheduling many repairs in a short time. */
        Common::TimeSpan RepairActionSchedulingInterval;

        /* RepairPolicyEngineService remembers the Repair History by reporting it as health property.
	        Then when new primary is elected the new primary waits this interval for the reports from previous primary to appear in health store.
	        Then it reads the RepairHistory back from health property . */
        Common::TimeSpan WaitForHealthStatesAfterBecomingPrimaryInterval;

        /* The default value can be tweaked if there are some unexpected delays when RepairPolicyEngineService talks to WinFab e.g. due to busy tenant. */
        Common::TimeSpan  WindowsFabricQueryTimeoutInterval;

        /* This is  frequency how often the RepairPolicyEngineService wakes up to check the health of nodes and schedule repairs. */
        Common::TimeSpan  HealthCheckInterval;

        /* TTL - time after which the health reported by RepairPolicyEngineService against itself is considered expired and therefore considered as error - e.g. in case The RepairPolicyEngineService hangs/freezes. */
        Common::TimeSpan ExpectedHealthRefreshInterval;
        
        const NodeRepairPolicyConfiguration& get_NodeRepairPolicyConfiguration() const
        {
            return nodeRepairPolicyConfiguration_;
        }

    private:
        static const uint DefaultMaxNumberOfRepairsToRemember = 10;
        static const int DefaultExpectedHealthRefreshIntervalInSeconds = 3600;

		/* Constants used in parameter names in Settings.xml */
		static const std::wstring HealthCheckIntervalInSecondsName;
		static const std::wstring ActionSchedulingIntervalInSecondsName;
		static const std::wstring MaxNumberOfRepairsToRememberName;
		static const std::wstring ExpectedHealthRefreshIntervalInSecondsName;
		static const std::wstring WaitForHealthUpdatesAfterBecomingPrimaryInSecondsName;
		static const std::wstring WindowsFabricQueryTimeoutInSecondsName;

		/* Private method to load the settings applied to the service */
		Common::ErrorCode RepairPolicyEngineConfiguration::LoadServiceSettings(Common::ComPointer<IFabricConfigurationPackage> fabricConfigurationPackageCPtr);
        NodeRepairPolicyConfiguration nodeRepairPolicyConfiguration_;
    };
}
