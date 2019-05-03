// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace UpgradeService
    {
        class UpgradeServiceConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(UpgradeServiceConfig, "UpgradeServiceConfig")

        public:
            // The PlacementConstraints for Upgrade service
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::NotAllowed);
            // The TargetReplicaSetSize for UpgradeService
            PUBLIC_CONFIG_ENTRY(int, L"UpgradeService", TargetReplicaSetSize, 3, Common::ConfigEntryUpgradePolicy::NotAllowed);
            // The MinReplicaSetSize for UpgradeService
            PUBLIC_CONFIG_ENTRY(int, L"UpgradeService", MinReplicaSetSize, 2, Common::ConfigEntryUpgradePolicy::NotAllowed);
            // The CoordinatorType for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", CoordinatorType, L"WUTest", Common::ConfigEntryUpgradePolicy::NotAllowed);
            //BaseUrl for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", BaseUrl, L"", Common::ConfigEntryUpgradePolicy::Static);			
            //ClusterId for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", ClusterId, L"", Common::ConfigEntryUpgradePolicy::Static);
            //X509StoreName for UpgradeService
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509StoreName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509StoreName, L"My", Common::ConfigEntryUpgradePolicy::Dynamic);
#endif
            //X509StoreLocation for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509StoreLocation, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //X509FindType for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509FindType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //X509FindValue for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509FindValue, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //X509SecondaryFindValue for UpgradeService 
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", X509SecondaryFindValue, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            // The interval between UpgradeService poll of SFRP
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", PollIntervalInSeconds, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);            

            // The TTL of health report sent by UpgradeService. The service will be marked as Error if the health report is not periodically updates within this interval
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", HealthReportTTLInSeconds, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The continuous failure count on which a warning health report is generated
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeService", ContinuousFailureWarningThreshold, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The continuous failure count on which a error health report is generated
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeService", ContinuousFailureErrorThreshold, 5, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The continuous failure count on which the service reports TransientFault
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeService", ContinuousFailureFaultThreshold, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

            //OnlyBaseUpgrade for UpgradeService
            PUBLIC_CONFIG_ENTRY(bool, L"UpgradeService", OnlyBaseUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);
            //TestCabFolder for UpgradeService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"UpgradeService", TestCabFolder, L"", Common::ConfigEntryUpgradePolicy::Static);

            // Uses TStore for persisted stateful storage when set to true
            INTERNAL_CONFIG_ENTRY(bool, L"UpgradeService", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);
            
            // Copy cluster package timeout in seconds
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", CopyClusterPackageTimeoutInSeconds, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Provision cluster package timeout in seconds
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", ProvisionClusterPackageTimeoutInSeconds, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Start cluster upgrade timeout in seconds
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", StartClusterUpgradeTimeoutInSeconds, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The interval between UpgradeService app poll of SFRP
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", AppPollIntervalInSeconds, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
            // The timeout for application package copy to ImageStore for SFRP managed resouces 
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"UpgradeService", ApplicationPackageCopyTimeoutInSeconds, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

            // Node status batch size used in the communication with SFRP.
            INTERNAL_CONFIG_ENTRY(int, L"UpgradeService", NodeStatusBatchSize, 25, Common::ConfigEntryUpgradePolicy::Dynamic);
        };
    }
}
