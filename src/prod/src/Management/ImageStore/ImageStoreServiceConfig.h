// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class ImageStoreServiceConfig : Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(ImageStoreServiceConfig, "ImageStoreService")         

            //The Enabled flag for ImageStoreService. Default: false.
            PUBLIC_CONFIG_ENTRY(bool, L"ImageStoreService", Enabled, false, Common::ConfigEntryUpgradePolicy::Static);
            // The TargetReplicaSetSize for ImageStoreService
            PUBLIC_CONFIG_ENTRY(int, L"ImageStoreService", TargetReplicaSetSize, 7, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for ImageStoreService
            PUBLIC_CONFIG_ENTRY(int, L"ImageStoreService", MinReplicaSetSize, 3, Common::ConfigEntryUpgradePolicy::Static);
            // The ReplicaRestartWaitDuration for ImageStoreService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreService", ReplicaRestartWaitDuration, Common::TimeSpan::FromSeconds(60.0 * 30), Common::ConfigEntryUpgradePolicy::Static);
            // The QuorumLossWaitDuration for ImageStoreService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreService", QuorumLossWaitDuration, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Static);
            // The StandByReplicaKeepDuration for ImageStoreService
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreService", StandByReplicaKeepDuration, Common::TimeSpan::FromSeconds(3600.0 * 24 * 7), Common::ConfigEntryUpgradePolicy::Static);
            // The PlacementConstraints for ImageStoreService
            PUBLIC_CONFIG_ENTRY(std::wstring, L"ImageStoreService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);

            // Timeout value for top-level upload request to Image Store Service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientUploadTimeout, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Timeout value for top-level copy request to Image Store Service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientCopyTimeout, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Timeout value for top-level download request to Image Store Service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientDownloadTimeout, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Timeout value for top-level list request to Image Store Service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientListTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Timeout value for all non-upload/non-download requests (e.g. exists, delete) to Image Store Service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientDefaultTimeout, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Default interval used to report progress information (<= 0 to disable)
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ImageStoreClient", ClientDefaultProgressReportingInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Delay between starting concurrent operations such as parallel downloading (used for testing)
            INTERNAL_CONFIG_ENTRY(int, L"ImageStoreClient", ClientOperationDelayMilliseconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Enable/Disable affinity of ImageStoreService to CM. Default: true.
            INTERNAL_CONFIG_ENTRY(bool, L"ImageStoreService", EnableClusterManagerAffinity, true, Common::ConfigEntryUpgradePolicy::Static);
        };
    }
}
