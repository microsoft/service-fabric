// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This explicit include allows a preprocessing script to expand
// the SL_CONFIG_PROPERTIES macro into individual CONFIG_ENTRY
// macros for the fabric setting CSV script to parse.
// (see src\prod\src\scripts\preprocess_config_macros.pl)
// 
#include "../../ServiceModel/data/txnreplicator/SLConfigMacros.h"

namespace Management
{
    namespace FileStoreService
    {
        class FileStoreServiceConfig : 
            public Common::ComponentConfig,
            public Reliability::ReplicationComponent::REConfigBase,
            public TxnReplicator::TRConfigBase,
            public TxnReplicator::SLConfigBase
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(FileStoreServiceConfig, "FileStoreService")

            // The timeout for performing naming operation
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", NamingOperationTimeout, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The timeout for performing query operation
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", QueryOperationTimeout, Common::TimeSpan::FromSeconds(60.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum number of parallel files that secondary can copy from primary. '0' == number of cores 
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", MaxCopyOperationThreads, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The maximum number of parallel threads allowed to perform FileOperations (Copy/Move) in the primary. '0' == number of cores 
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", MaxFileOperationThreads, 100, Common::ConfigEntryUpgradePolicy::Static);
            // The maximum number of parallel store transaction operations allowed on primary. '0' == number of cores 
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", MaxStoreOperations, 4096, Common::ConfigEntryUpgradePolicy::Static);
            // The maximum number of parallel threads allowed to process requests in the primary. '0' == number of cores 
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", MaxRequestProcessingThreads, 200, Common::ConfigEntryUpgradePolicy::Static);
            // The maximum number of file copy retries on the secondary before giving up
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", MaxSecondaryFileCopyFailureThreshold, 25, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The file copy retry delay (in milliseconds)
            PUBLIC_CONFIG_ENTRY(uint, L"FileStoreService", SecondaryFileCopyRetryDelayMilliseconds, 500, Common::ConfigEntryUpgradePolicy::Dynamic);
            // Enable/Disable anonymous access to the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(bool, L"FileStoreService", AnonymousAccessEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

            // The primary AccountType of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountType, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The primary account Username of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountUserName, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The primary account password of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(Common::SecureString, L"FileStoreService", PrimaryAccountUserPassword, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
            // The password secret which used as seed to generated same password when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(Common::SecureString, L"FileStoreService", PrimaryAccountNTLMPasswordSecret, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
            // The store location of the X509 certificate used to generate HMAC on the PrimaryAccountNTLMPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountNTLMX509StoreLocation, L"LocalMachine", Common::ConfigEntryUpgradePolicy::Static);
            // The store name of the X509 certificate used to generate HMAC on the PrimaryAccountNTLMPasswordSecret when using NTLM authentication
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountNTLMX509StoreName, L"", Common::ConfigEntryUpgradePolicy::Static);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountNTLMX509StoreName, L"MY", Common::ConfigEntryUpgradePolicy::Static);
#endif
	        // The thumbprint of the X509 certificate used to generate HMAC on the PrimaryAccountNTLMPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", PrimaryAccountNTLMX509Thumbprint, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The password secret which used as seed to generated same password when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(Common::SecureString, L"FileStoreService", CommonNameNtlmPasswordSecret, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);

            // The store location of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName1Ntlmx509StoreLocation, L"LocalMachine", Common::ConfigEntryUpgradePolicy::Static);
            // The store name of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName1Ntlmx509StoreName, L"", Common::ConfigEntryUpgradePolicy::Static);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName1Ntlmx509StoreName, L"MY", Common::ConfigEntryUpgradePolicy::Static);
#endif
            // The common name of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName1Ntlmx509CommonName, L"", Common::ConfigEntryUpgradePolicy::Static);
            // Specifies whether to generate an account with user name V1 generation algorithm.
            // Starting with Service Fabric version 6.1, an account with v2 generation is always created. The V1 account is necessary for upgrades from/to versions that do not support V2 generation (prior to 6.1).
            PUBLIC_CONFIG_ENTRY(bool, L"FileStoreService", GenerateV1CommonNameAccount, true, Common::ConfigEntryUpgradePolicy::Static);

            // The secondary AccountType of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountType, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The secondary account Username of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountUserName, L"", Common::ConfigEntryUpgradePolicy::Static);
            // The secondary account password of the principal to ACL the FileStoreService shares
            PUBLIC_CONFIG_ENTRY(Common::SecureString, L"FileStoreService", SecondaryAccountUserPassword, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
            // The password secret which used as seed to generated same password when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(Common::SecureString, L"FileStoreService", SecondaryAccountNTLMPasswordSecret, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
            // The store location of the X509 certificate used to generate HMAC on the SecondaryAccountNTLMPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountNTLMX509StoreLocation, L"LocalMachine", Common::ConfigEntryUpgradePolicy::Static);
            // The store name of the X509 certificate used to generate HMAC on the SecondaryAccountNTLMPasswordSecret when using NTLM authentication
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountNTLMX509StoreName, L"", Common::ConfigEntryUpgradePolicy::Static);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountNTLMX509StoreName, L"MY", Common::ConfigEntryUpgradePolicy::Static);
#endif
            // The thumbprint of the X509 certificate used to generate HMAC on the SecondaryAccountNTLMPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", SecondaryAccountNTLMX509Thumbprint, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The store location of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName2Ntlmx509StoreLocation, L"LocalMachine", Common::ConfigEntryUpgradePolicy::Static);
            // The store name of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
#ifdef PLATFORM_UNIX
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName2Ntlmx509StoreName, L"", Common::ConfigEntryUpgradePolicy::Static);
#else
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName2Ntlmx509StoreName, L"MY", Common::ConfigEntryUpgradePolicy::Static);
#endif
            // The common name of the X509 certificate used to generate HMAC on the CommonNameNtlmPasswordSecret when using NTLM authentication
            PUBLIC_CONFIG_ENTRY(std::wstring, L"FileStoreService", CommonName2Ntlmx509CommonName, L"", Common::ConfigEntryUpgradePolicy::Static);

            // The flag to determine whether to use new version of upload protocol based on chunking file contents introduced in v6.3. This protocol version provides better performance and reliability compared to previous versions.
            PUBLIC_CONFIG_ENTRY(bool, L"FileStoreService", AcceptChunkUpload, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Config for disabling NTLM authentication in the ImageStoreService
            INTERNAL_CONFIG_ENTRY(bool, L"FileStoreService", DisableNtlmAuthentication, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The flag for using the new version of the upload protocol introduced in v6.4. This protocol version uses service fabric transport to upload files to image store which provides better performance than SMB protocol used in previous versions.
            PUBLIC_CONFIG_ENTRY(bool, L"FileStoreService", UseChunkContentInTransportMessage, true, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The period of time when generated simple transactions are batched. To disable batching, pass 0.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", CommitBatchingPeriod, 50, Common::ConfigEntryUpgradePolicy::Static);
            // The limit for simple transaction batch. When batched replications size reach this limit, the store will start a new group for new simple transaction.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", CommitBatchingSizeLimitInKB, 10, Common::ConfigEntryUpgradePolicy::Static);
            // When the number of pending completion transaction is less or equal the low watermark, new simple transactions will not be batched. -1 to disable.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", TransactionLowWatermark, 25, Common::ConfigEntryUpgradePolicy::Static);
            // When the number of pending completion transaction is greater or equal the high watermark, batching period will be extended every time the period elapses. -1 to disable.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", TransactionHighWatermark, -1, Common::ConfigEntryUpgradePolicy::Static);
            // The batch period extension. When batching period needs to be extended, it will be extended this much every time.  0 means extending CommitBatchingPeriod.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", CommitBatchingPeriodExtension, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The store will throttle operations once the number of operations in the replication queue reaches this value
            INTERNAL_CONFIG_ENTRY(int64, L"FileStoreService", ThrottleOperationCount, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The store will throttle operations once the memory utilization (bytes) of the replication queue reaches this value
            INTERNAL_CONFIG_ENTRY(int64, L"FileStoreService", ThrottleQueueSizeBytes, 0, Common::ConfigEntryUpgradePolicy::Static);
            // The max cursors for ese store
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", MaxCursors, 16384, Common::ConfigEntryUpgradePolicy::Static);

            // The maximum number of parallel threads allowed during upload/download of files in the client. '0' == number of cores
            INTERNAL_CONFIG_ENTRY(uint, L"FileStoreService", MaxClientOperationThreads, 25, Common::ConfigEntryUpgradePolicy::Static);
            // The backoff interval for StoreTransaction failures
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", StoreTransactionRetryInterval, Common::TimeSpan::FromSeconds(3.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The max backoff interval for StoreTransaction failures
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", StoreTransactionMaxRetryInterval, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The backoff interval for Recovery failure
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", RecoveryRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);            
            // Timeout for getting the primary staging location on client
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", GetStagingLocationTimeout, Common::TimeSpan::FromSeconds(30.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // Retry attempt for getting the staging location from FSS primary
            INTERNAL_CONFIG_ENTRY(uint, L"FileStoreService", MaxGetStagingLocationRetryAttempt, 5, Common::ConfigEntryUpgradePolicy::Dynamic);
            // Timeout for getting the primary staging location on client
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", InternalServiceCallTimeout, Common::TimeSpan::FromSeconds(90.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The backoff interval for client-side retries
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", ClientRetryInterval, Common::TimeSpan::FromSeconds(5.0), Common::ConfigEntryUpgradePolicy::Dynamic);            
            // The max retry count on file operation failure
            INTERNAL_CONFIG_ENTRY(uint, L"FileStoreService", MaxFileOperationFailureRetryCount, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The backoff interval for file operations
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", FileOperationBackoffInterval, Common::TimeSpan::FromSeconds(1.0), Common::ConfigEntryUpgradePolicy::Dynamic);
            //The timeout for upload session
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", UploadSessionTimeout, Common::TimeSpan::FromMinutes(30), Common::ConfigEntryUpgradePolicy::Dynamic);
            //The interval of processing the next cleanup of upload sessions
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", ProcessPendingCleanupInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);
            // The max retry count on staging file SMB copy to the FSS primary staging location
            INTERNAL_CONFIG_ENTRY(uint, L"FileStoreService", MaxStagingCopyFailureRetryCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The store will be auto-compacted during open when the database file size exceeds this threshold (<=0 to disable)
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", CompactionThresholdInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic);

            // The ESE max cache size, which controls available buffers. Default 1024 MB. Use -1 to use ESE defaults.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", MaxCacheSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static);
            // The ESE min cache size, which controls available buffers. Default 1024 MB. Use -1 to use ESE defaults.
            INTERNAL_CONFIG_ENTRY(int, L"FileStoreService", MinCacheSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static);

            //Timeout value for short requests (e.g. exists, list, delete) to file store service
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", ShortRequestTimeout, Common::TimeSpan::FromMinutes(2), Common::ConfigEntryUpgradePolicy::Dynamic);

            // Uses TStore for persisted stateful storage when set to true
            INTERNAL_CONFIG_ENTRY(bool, L"FileStoreService", EnableTStore, false, Common::ConfigEntryUpgradePolicy::Static);
            // Enables extra tracing in store transaction processing job queue
            INTERNAL_CONFIG_ENTRY(bool, L"FileStoreService", EnableTxQueueTracing, false, Common::ConfigEntryUpgradePolicy::Static);
            // Enables logging of additional disk related data for diagnostics purposes
            TEST_CONFIG_ENTRY(bool, L"FileStoreService", EnableLoggingOfDiskSpace, false, Common::ConfigEntryUpgradePolicy::Dynamic);
            // The time interval between logging of free disk space
            TEST_CONFIG_ENTRY(Common::TimeSpan, L"FileStoreService", FreeDiskSpaceLoggingInterval, Common::TimeSpan::FromSeconds(15.0), Common::ConfigEntryUpgradePolicy::Dynamic);

            // Config for enabling Chaos during upload
            TEST_CONFIG_ENTRY(bool, L"FileStoreService", EnableChaosDuringFileUpload, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            // Config for enabling automatic recovery after dataloss when a threshold retry attempt is exceeded for copying missing files.
            TEST_CONFIG_ENTRY(bool, L"FileStoreService", EnableAutomaticRecoveryAfterDataloss, false, Common::ConfigEntryUpgradePolicy::Dynamic);

            //Config threshold for retrying missing file copy in primary's recovery operation
            TEST_CONFIG_ENTRY(uint, L"FileStoreService", RetryMissingCopyFileCountDuringRecovery, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

            RE_INTERNAL_CONFIG_PROPERTIES(L"FileStoreService/Replication", 0, 8192, 314572800, 16384, 314572800);
            TR_INTERNAL_CONFIG_PROPERTIES(L"FileStoreService/TransactionalReplicator2");
            SL_CONFIG_PROPERTIES(L"FileStoreService/SharedLog");
        };
    }
}
