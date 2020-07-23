// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreService
    {
        class BackupRestoreServiceConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(BackupRestoreServiceConfig, "BackupRestoreService")

        public:

            bool static IsBackupRestoreServiceConfigured();

            // The PlacementConstraints for BackupRestore service
            PUBLIC_CONFIG_ENTRY(std::wstring, L"BackupRestoreService", PlacementConstraints, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The TargetReplicaSetSize for BackupRestoreService
            PUBLIC_CONFIG_ENTRY(int, L"BackupRestoreService", TargetReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The MinReplicaSetSize for BackupRestoreService
            PUBLIC_CONFIG_ENTRY(int, L"BackupRestoreService", MinReplicaSetSize, 0, Common::ConfigEntryUpgradePolicy::Static);

            // This indicates the certificate to use for encryption and decryption of creds
            // Name of X.509 certificate store that is used for encrypting decrypting store credentials used by Backup Restore service
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"BackupRestoreService", SecretEncryptionCertX509StoreName, L"", ConfigEntryUpgradePolicy::Dynamic);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"BackupRestoreService", SecretEncryptionCertX509StoreName, L"My", ConfigEntryUpgradePolicy::Dynamic);
#endif
            // Thumbprint of the Secret encryption X509 certificate
            PUBLIC_CONFIG_ENTRY(std::wstring, L"BackupRestoreService", SecretEncryptionCertThumbprint, L"", ConfigEntryUpgradePolicy::Dynamic);
            // This represents timeout for the API calls between BackupRestore service and user service host
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", ApiTimeoutInSeconds, 60, Common::ConfigEntryUpgradePolicy::Static);
            // This represents timeout for the Backup Restore store operation
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", StoreApiTimeoutInSeconds, 3600, Common::ConfigEntryUpgradePolicy::Static);
            // This represents interval for retry for API calls from user service host to Backup Restore service
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", ApiRetryIntervalInSeconds, 15, Common::ConfigEntryUpgradePolicy::Static);
            // This represents maximum interval for retry for API calls from user service host to Backup Restore service
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", MaxApiRetryIntervalInSeconds, 60, Common::ConfigEntryUpgradePolicy::Static);
            // This represents maximum retry attempts in case of API invocation failure
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", MaxApiRetryCount, 3, Common::ConfigEntryUpgradePolicy::Static);
            // This represents interval for retry for API calls from user service host to Backup Restore service
            INTERNAL_CONFIG_ENTRY(bool, L"BackupRestoreService", EnableCompression, true, Common::ConfigEntryUpgradePolicy::Static);
            // This represents the default jitter added/ removed to the backup time
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", JitterInBackupsInSeconds, 10, Common::ConfigEntryUpgradePolicy::Static);

            // This represents the timeout for each internal work item in Backup Restore service
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", WorkItemTimeoutInSeconds, 300, Common::ConfigEntryUpgradePolicy::Static);

            // Thread count throttle for BackupCopier proxy job queue on upload/download requests
            INTERNAL_CONFIG_ENTRY(int, L"BackupRestoreService", BackupCopierJobQueueThrottle, 25, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The amount of time to allow for BackupCopier specific timeout errors to return to the client. If this buffer is too small, then the client times out before the server and gets a generic timeout error.
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"BackupRestoreService", BackupCopierTimeoutBuffer, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Dynamic);

            // The DsmsAutopilotServiceName for BackupRestore service
            INTERNAL_CONFIG_ENTRY(std::wstring, L"BackupRestoreService", DsmsAutopilotServiceName, L"", Common::ConfigEntryUpgradePolicy::Static);
        };
    }
}
