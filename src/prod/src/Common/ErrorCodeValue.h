    // ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{ 
    namespace ErrorCodeValue
    {
        // Without casting FABRIC_E_ error codes to int, Clang will use consider them unsigned,
        // and since E_INVALIDARG etc are HRESULT which is LONG, the size of Enum will become 8
        // bytes, which will be different from the Windows side.
        enum Enum : int
        {
            // Public ErrorCodes
            Success = S_OK,
            InvalidArgument = E_INVALIDARG,
            AccessDenied = E_ACCESSDENIED,
            ArgumentNull = E_POINTER,
            FabricComponentAborted = (int)FABRIC_E_OBJECT_CLOSED,
            GatewayUnreachable = (int)FABRIC_E_GATEWAY_NOT_REACHABLE,
            UserRoleClientCertificateNotConfigured = (int)FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED,
            HostingServiceTypeAlreadyRegistered = (int)FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED,
            HostingServiceTypeNotRegistered = (int)FABRIC_E_SERVICE_TYPE_NOT_REGISTERED,
            InvalidAddress = (int)FABRIC_E_INVALID_ADDRESS,
            InvalidMessage = (int)FABRIC_E_COMMUNICATION_ERROR,
            MessageTooLarge = (int)FABRIC_E_MESSAGE_TOO_LARGE,
            ConnectionClosedByRemoteEnd = (int)FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END,
            InvalidNameUri = (int)FABRIC_E_INVALID_NAME_URI,
            InvalidNamingAction = (int)FABRIC_E_COMMUNICATION_ERROR,
            InvalidServicePartition = (int)FABRIC_E_INVALID_PARTITION_KEY,
            InvalidState = (int)FABRIC_E_INVALID_OPERATION,
            NameAlreadyExists = (int)FABRIC_E_NAME_ALREADY_EXISTS,
            NameNotEmpty = (int)FABRIC_E_NAME_NOT_EMPTY,
            NameNotFound = (int)FABRIC_E_NAME_DOES_NOT_EXIST,
            NodeNotFound = (int)FABRIC_E_NODE_NOT_FOUND,
            NodeIsUp = (int)FABRIC_E_NODE_IS_UP,
            NotPrimary = (int)FABRIC_E_NOT_PRIMARY,
            NotReady = (int)FABRIC_E_NOT_READY,
            NoWriteQuorum = (int)FABRIC_E_NO_WRITE_QUORUM,
            ObjectClosed = (int)FABRIC_E_OBJECT_CLOSED,
            OperationCanceled = E_ABORT,
            OperationFailed = E_FAIL,
            AsyncOperationNotComplete = (int)FABRIC_E_OPERATION_NOT_COMPLETE,
            OutOfMemory = E_OUTOFMEMORY,
            NotImplemented = E_NOTIMPL,
            PropertyNotFound = (int)FABRIC_E_PROPERTY_DOES_NOT_EXIST,
            PropertyTooLarge = (int)FABRIC_E_VALUE_TOO_LARGE,
            PropertyValueEmpty = (int)FABRIC_E_VALUE_EMPTY,
            PropertyCheckFailed = (int)FABRIC_E_PROPERTY_CHECK_FAILED,
            EntryTooLarge = (int)FABRIC_E_VALUE_TOO_LARGE,
            REInvalidEpoch = E_INVALIDARG, // do we need to round trip this through HRESULT?
            ReconfigurationPending = (int)FABRIC_E_RECONFIGURATION_PENDING,
            REQueueFull = (int)FABRIC_E_REPLICATION_QUEUE_FULL,
            REReplicaAlreadyExists = (int)FABRIC_E_INVALID_OPERATION, // do we need to round trip this through HRESULT?
            REReplicaDoesNotExist = (int)FABRIC_E_REPLICA_DOES_NOT_EXIST,
            ReplicaDoesNotExist = (int)FABRIC_E_REPLICA_DOES_NOT_EXIST,
            ServiceNotFound = (int)FABRIC_E_SERVICE_DOES_NOT_EXIST,
            ServiceOffline = (int)FABRIC_E_SERVICE_OFFLINE,
            ServiceMetadataMismatch = (int)FABRIC_E_SERVICE_METADATA_MISMATCH,
            ServiceAffinityChainNotSupported = (int)FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED,
            SqlStoreDuplicateInsert = (int)FABRIC_E_WRITE_CONFLICT,
            SqlStoreUnableToConnect = (int)FABRIC_E_COMMUNICATION_ERROR,
            StoreBufferTruncated = (int)FABRIC_E_VALUE_TOO_LARGE,
            StoreWriteConflict = (int)FABRIC_E_WRITE_CONFLICT,
            StoreRecordNotFound = (int)FABRIC_E_KEY_NOT_FOUND,
            StoreRecordAlreadyExists = (int)FABRIC_E_WRITE_CONFLICT,
            StoreKeyTooLarge = (int)FABRIC_E_KEY_TOO_LARGE,
            StoreSequenceNumberCheckFailed = (int)FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED,
            StoreTransactionNotActive = (int)FABRIC_E_TRANSACTION_NOT_ACTIVE,
            StoreTransactionTooLarge = (int)FABRIC_E_TRANSACTION_TOO_LARGE,
            Timeout = (int)FABRIC_E_TIMEOUT,
            UnsupportedNamingOperation = (int)FABRIC_E_COMMUNICATION_ERROR,
            UserServiceAlreadyExists = (int)FABRIC_E_SERVICE_ALREADY_EXISTS,
            UserServiceNotFound = (int)FABRIC_E_SERVICE_DOES_NOT_EXIST,
            UserServiceGroupAlreadyExists = (int)FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS,
            UserServiceGroupNotFound = (int)FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST,
            EnumerationCompleted = (int)FABRIC_E_ENUMERATION_COMPLETED,
            ApplicationTypeProvisionInProgress = (int)FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS,
            ApplicationTypeAlreadyExists = (int)FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS,
            ApplicationTypeNotFound = (int)FABRIC_E_APPLICATION_TYPE_NOT_FOUND,
            ApplicationTypeInUse = (int)FABRIC_E_APPLICATION_TYPE_IN_USE,
            ApplicationAlreadyExists = (int)FABRIC_E_APPLICATION_ALREADY_EXISTS,
            ApplicationNotFound = (int)FABRIC_E_APPLICATION_NOT_FOUND,
            ApplicationUpgradeInProgress = (int)FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS,
            ApplicationNotUpgrading = (int)FABRIC_E_APPLICATION_NOT_UPGRADING,
            ApplicationAlreadyInTargetVersion = (int)FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION,
            ApplicationUpgradeValidationError = (int)FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR,
            ApplicationUpdateInProgress = (int)FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS,
            ServiceTypeNotFound = (int)FABRIC_E_SERVICE_TYPE_NOT_FOUND,
            ServiceTypeMismatch = (int)FABRIC_E_SERVICE_TYPE_MISMATCH,
            ServiceTypeTemplateNotFound = (int)FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND,
            ConfigurationSectionNotFound = (int)FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND,
            ConfigurationParameterNotFound = (int)FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND,
            InvalidConfiguration = (int)FABRIC_E_INVALID_CONFIGURATION,
            ImageBuilderValidationError = (int)FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR,
            FileNotFound = (int)FABRIC_E_FILE_NOT_FOUND,
            DirectoryNotFound = (int)FABRIC_E_DIRECTORY_NOT_FOUND,
            InvalidDirectory = (int)FABRIC_E_INVALID_DIRECTORY,
            PathTooLong = (int)FABRIC_E_PATH_TOO_LONG,
            ImageStoreIOError = (int)FABRIC_E_IMAGESTORE_IOERROR,
            AcquireFileLockFailed = (int)FABRIC_E_ACQUIRE_FILE_LOCK_FAILED,
            ImageBuilderUnexpectedError = (int)FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR,
            CorruptedImageStoreObjectFound = (int)FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND,
            PartitionNotFound = (int)FABRIC_E_PARTITION_NOT_FOUND,
            ProcessDeactivated = (int)FABRIC_E_PROCESS_DEACTIVATED,
            ProcessAborted = (int)FABRIC_E_PROCESS_ABORTED,
            UpgradeFailed = (int)FABRIC_E_UPGRADE_FAILED,
            UpgradeDomainAlreadyCompleted = (int)FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED,
            InvalidCredentialType = (int)FABRIC_E_INVALID_CREDENTIAL_TYPE,
            InvalidX509FindType = (int)FABRIC_E_INVALID_X509_FIND_TYPE,
            InvalidX509StoreLocation = (int)FABRIC_E_INVALID_X509_STORE_LOCATION,
            InvalidX509StoreName = (int)FABRIC_E_INVALID_X509_STORE_NAME,
            InvalidX509Thumbprint = (int)FABRIC_E_INVALID_X509_THUMBPRINT,
            InvalidX509NameList = (int)FABRIC_E_INVALID_X509_NAME_LIST,
            InvalidProtectionLevel = (int)FABRIC_E_INVALID_PROTECTION_LEVEL,
            InvalidX509Store = (int)FABRIC_E_INVALID_X509_STORE,
            InvalidSubjectName = (int)FABRIC_E_INVALID_SUBJECT_NAME,
            InvalidAllowedCommonNameList = (int)FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST,
            InvalidCredentials = (int)FABRIC_E_INVALID_CREDENTIALS,
            DecryptionFailed = (int)FABRIC_E_DECRYPTION_FAILED,
            ConfigurationPackageNotFound = (int)FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND,
            DataPackageNotFound = (int)FABRIC_E_DATA_PACKAGE_NOT_FOUND,
            CodePackageNotFound = (int)FABRIC_E_CODE_PACKAGE_NOT_FOUND,
            EndpointResourceNotFound = (int)FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND,
            InvalidOperation = (int)FABRIC_E_INVALID_OPERATION,
            FabricVersionNotFound = (int)FABRIC_E_FABRIC_VERSION_NOT_FOUND,
            FabricVersionInUse = (int)FABRIC_E_FABRIC_VERSION_IN_USE,
            FabricVersionAlreadyExists = (int)FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS,
            FabricAlreadyInTargetVersion = (int)FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION,
            FabricNotUpgrading = (int)FABRIC_E_FABRIC_NOT_UPGRADING,
            FabricUpgradeInProgress = (int)FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS,
            FabricUpgradeValidationError = (int)FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR,
            HealthMaxReportsReached = (int)FABRIC_E_HEALTH_MAX_REPORTS_REACHED,
            HealthStaleReport = (int)FABRIC_E_HEALTH_STALE_REPORT,
            EncryptionFailed = (int)FABRIC_E_ENCRYPTION_FAILED,
            InvalidAtomicGroup = (int)FABRIC_E_INVALID_ATOMIC_GROUP,
            HealthEntityNotFound = (int)FABRIC_E_HEALTH_ENTITY_NOT_FOUND,
            ServiceManifestNotFound = (int)FABRIC_E_SERVICE_MANIFEST_NOT_FOUND,
            ReliableSessionTransportStartupFailure = (int)FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE,
            ReliableSessionAlreadyExists = (int)FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS,
            ReliableSessionCannotConnect = (int)FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT,
            ReliableSessionManagerExists = (int)FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS,
            ReliableSessionRejected = (int)FABRIC_E_RELIABLE_SESSION_REJECTED,
            ReliableSessionNotFound = (int)FABRIC_E_RELIABLE_SESSION_NOT_FOUND,
            ReliableSessionQueueEmpty = (int)FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY,
            ReliableSessionQuotaExceeded = (int)FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED,
            ReliableSessionServiceFaulted = (int)FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED,
            ReliableSessionManagerAlreadyListening = (int)FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING,
            ReliableSessionManagerNotFound = (int)FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND,
            ReliableSessionManagerNotListening = (int)FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING,
            ReliableSessionInvalidTargetPartition = (int)FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION,
            InvalidServiceType = (int)FABRIC_E_INVALID_SERVICE_TYPE,
            ImageBuilderTimeout = (int)FABRIC_E_IMAGEBUILDER_TIMEOUT,
            ImageBuilderAccessDenied = (int)FABRIC_E_IMAGEBUILDER_ACCESS_DENIED,
            ImageBuilderInvalidMsiFile = (int)FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE,
            ServiceTooBusy = (int)FABRIC_E_SERVICE_TOO_BUSY,
            RepairTaskAlreadyExists = (int)FABRIC_E_REPAIR_TASK_ALREADY_EXISTS,
            RepairTaskNotFound = (int)FABRIC_E_REPAIR_TASK_NOT_FOUND,
            REOperationTooLarge = (int)FABRIC_E_REPLICATION_OPERATION_TOO_LARGE,
            InstanceIdMismatch = (int)FABRIC_E_INSTANCE_ID_MISMATCH,
            NodeHasNotStoppedYet = (int)FABRIC_E_NODE_HAS_NOT_STOPPED_YET,
            InsufficientClusterCapacity = (int)FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY,
            ConstraintKeyUndefined = (int)FABRIC_E_CONSTRAINT_KEY_UNDEFINED,
            ConstraintNotSatisfied = (int)FABRIC_E_CONSTRAINT_NOT_SATISFIED,
            VerboseFMPlacementHealthReportingRequired = (int)FABRIC_E_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED,
            InvalidPackageSharingPolicy = (int)FABRIC_E_INVALID_PACKAGE_SHARING_POLICY,
            PreDeploymentNotAllowed = (int)FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED,
            PLBNotReady = (int)FABRIC_E_LOADBALANCER_NOT_READY,
            ConnectionDenied = (int)FABRIC_E_CONNECTION_DENIED,
            ServerAuthenticationFailed = (int)FABRIC_E_SERVER_AUTHENTICATION_FAILED,
            MultithreadedTransactionsNotAllowed = (int)FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED,
            TransactionAborted = (int)FABRIC_E_TRANSACTION_ABORTED,
            CertificateNotFound = (int)FABRIC_E_CERTIFICATE_NOT_FOUND,

            /// <summary>
            /// Invalid backup setting. E.g. incremental backup option is not set upfront etc.
            /// </summary>
            InvalidBackupSetting = (int)FABRIC_E_INVALID_BACKUP_SETTING,

            InvalidRestoreData = (int)FABRIC_E_INVALID_RESTORE_DATA,

            RestoreSafeCheckFailed = (int)FABRIC_E_RESTORE_SAFE_CHECK_FAILED,

            DuplicateBackups = (int)FABRIC_E_DUPLICATE_BACKUPS,

            InvalidBackupChain = (int)FABRIC_E_INVALID_BACKUP_CHAIN,

            InvalidBackup = (int)FABRIC_E_INVALID_BACKUP,

            /// <summary>
            /// Incremental backups can only be done after an initial full backup.
            /// </summary>
            MissingFullBackup = (int)FABRIC_E_MISSING_FULL_BACKUP,

            DuplicateServiceNotificationName = (int)FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME,

            // Cannot restart a volatile replica
            InvalidReplicaOperation = (int)FABRIC_E_INVALID_REPLICA_OPERATION,

            // Replica is closing or opening
            InvalidReplicaStateForReplicaOperation = (int)FABRIC_E_INVALID_REPLICA_STATE,

            /// <summary>
            /// A backup is currently in progress.
            /// </summary>
            BackupInProgress = (int)FABRIC_E_BACKUP_IN_PROGRESS,

            /// <summary>
            ///cannot use moveprimary or movesecondary for stateless service
            /// </summary>
            InvalidPartitionOperation = (int)FABRIC_E_INVALID_PARTITION_OPERATION,
            AlreadyPrimaryReplica = (int)FABRIC_E_PRIMARY_ALREADY_EXISTS,
            AlreadySecondaryReplica = (int)FABRIC_E_SECONDARY_ALREADY_EXISTS,

            /// <summary>
            /// The backup directory is not empty.
            /// </summary>
            BackupDirectoryNotEmpty = (int)FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY,

            // Cannot force remove adhoc/system service replica. Force
            ForceNotSupportedForReplicaOperation = (int)FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION,

            //CannotConnect Error From ServiceCommunication
            ServiceCommunicationCannotConnect = (int)FABRIC_E_CANNOT_CONNECT,
            EndpointNotFound = (int)FABRIC_E_ENDPOINT_NOT_FOUND,
            SendFailed = (int)FABRIC_E_COMMUNICATION_ERROR,

            /// <summary>
            /// Deletion of backup files/directory failed. Currently this can happen
            /// in a scenario where backup is used mainly to truncate logs.
            /// </summary>
            DeleteBackupFileFailed = (int)FABRIC_E_DELETE_BACKUP_FILE_FAILED,

            InvalidTestCommandState = (int)FABRIC_E_INVALID_TEST_COMMAND_STATE,

            TestCommandOperationIdAlreadyExists = (int)FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS,

            /// <summary>
            /// Creation or deletion terminated due to persistent failures after bounded retry.
            /// </summary>
            CMOperationFailed = (int)FABRIC_E_CM_OPERATION_FAILED,

            ImageBuilderReservedDirectoryError = (int)FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR,

            ChaosAlreadyRunning = (int)FABRIC_E_CHAOS_ALREADY_RUNNING,

            FabricDataRootNotFound = (int)FABRIC_E_FABRIC_DATA_ROOT_NOT_FOUND,

            StopInProgress = (int)FABRIC_E_STOP_IN_PROGRESS,

            AlreadyStopped = (int)FABRIC_E_ALREADY_STOPPED,

            NodeIsDown = (int)FABRIC_E_NODE_IS_DOWN,

            NodeTransitionInProgress = (int)FABRIC_E_NODE_TRANSITION_IN_PROGRESS,

            InvalidInstanceId = (int)FABRIC_E_INVALID_INSTANCE_ID,

            InvalidDuration = (int)FABRIC_E_INVALID_DURATION,

            UploadSessionRangeNotSatisfiable = (int)FABRIC_E_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE,

            UploadSessionIdConflict = (int)FABRIC_E_UPLOAD_SESSION_ID_CONFLICT,

            ConfigUpgradeFailed = (int)FABRIC_E_CONFIG_UPGRADE_FAILED,

            InvalidPartitionSelector = (int)FABRIC_E_INVALID_PARTITION_SELECTOR,

            InvalidReplicaSelector = (int)FABRIC_E_INVALID_REPLICA_SELECTOR,

            DnsServiceNotFound = (int)FABRIC_E_DNS_SERVICE_NOT_FOUND,

            InvalidDnsName = (int)FABRIC_E_INVALID_DNS_NAME,

            DnsNameInUse = (int)FABRIC_E_DNS_NAME_IN_USE,

            ComposeDeploymentAlreadyExists = (int)FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS,

            ComposeDeploymentNotFound = (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND,

            ComposeDeploymentNotUpgrading = (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING,

            InvalidForStatefulServices = (int)FABRIC_E_INVALID_FOR_STATEFUL_SERVICES,

            InvalidForStatelessServices = (int)FABRIC_E_INVALID_FOR_STATELESS_SERVICES,

            OnlyValidForStatefulPersistentServices = (int)FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES,

            ContainerNotFound = (int)FABRIC_E_CONTAINER_NOT_FOUND,

            CentralSecretServiceGenericError = (int)FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC,
            SecretInvalid = (int)FABRIC_E_SECRET_INVALID,
            SecretTypeCannotBeChanged = (int)FABRIC_E_SECRET_TYPE_CANNOT_BE_CHANGED,
            SecretVersionAlreadyExists = (int)FABRIC_E_SECRET_VERSION_ALREADY_EXISTS,

            InvalidUploadSessionId = (int)FABRIC_E_INVALID_UPLOAD_SESSION_ID,

            BackupNotEnabled = (int)FABRIC_E_BACKUP_NOT_ENABLED,
            BackupEnabled = (int)FABRIC_E_BACKUP_IS_ENABLED,
            BackupPolicyDoesNotExist = (int)FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST,
            BackupPolicyAlreayExists = (int)FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS,
            RestoreAlreadyInProgress = (int)FABRIC_E_RESTORE_IN_PROGRESS,
            RestoreSourceTargetPartitionMismatch = (int)FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH,
            FaultAnalysisServiceNotEnabled = (int)FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED,

            BackupCopierUnexpectedError = (int)FABRIC_E_BACKUPCOPIER_UNEXPECTED_ERROR,
            BackupCopierTimeout = (int)FABRIC_E_BACKUPCOPIER_TIMEOUT,
            BackupCopierAccessDenied = (int)FABRIC_E_BACKUPCOPIER_ACCESS_DENIED,
            SingleInstanceApplicationAlreadyExists = (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS,
            SingleInstanceApplicationNotFound = (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND,
            SingleInstanceApplicationUpgradeInProgress = (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS,
            VolumeAlreadyExists = (int)FABRIC_E_VOLUME_ALREADY_EXISTS,
            VolumeNotFound = (int)FABRIC_E_VOLUME_NOT_FOUND,
            InvalidServiceScalingPolicy = (int)FABRIC_E_INVALID_SERVICE_SCALING_POLICY,

            DatabaseMigrationInProgress = (int)FABRIC_E_DATABASE_MIGRATION_IN_PROGRESS,
            
            NetworkNotFound = (int)FABRIC_E_NETWORK_NOT_FOUND,
            NetworkInUse = (int)FABRIC_E_NETWORK_IN_USE,
            EndpointNotReferenced = (int)FABRIC_E_ENDPOINT_NOT_REFERENCED,

            OperationNotSupported = (int)FABRIC_E_OPERATION_NOT_SUPPORTED,

            // ----------------------------------------------------------------
            // Internal ErrorCodes
            //
            // By starting internal error codes at the end of our reserved range minus count, ErrorCodeValues
            // remain consistent and do not cause gaps in the public error code range.
            //
            // This requires keeping this count in sync with the number of internal error codes.  If you get
            // a CIT error about this, update the following line as directed by the CIT output:

            INTERNAL_ERROR_CODE_COUNT = 188, // <-- Update this number when adding new internal error codes

            FIRST_INTERNAL_ERROR_CODE_MINUS_ONE = (int)FABRIC_E_LAST_RESERVED_HRESULT - INTERNAL_ERROR_CODE_COUNT,

            //
            // Add new internal error codes for the current release only between the
            // [Start ...] and [End ...] tags below.
            //
            // New internal error codes should always be added above existing internal error codes.
            // For example:
            //
            // * Add new error code here
            // ExistingErrorCode1
            // ExistingErrorCode2
            // * Do NOT add new error code here
            //
            // The file 'scripts\ErrorCodeValue.baseline' needs to be updated at the end of
            // each release to set the new threshold for internal error code changes.
            // The baseline file is used to verify internal error codes during
            // compile time to ensure that their ordering hasn't changed.
            //
            // Re-ordering internal error codes will cause their actual int value to
            // change between releases, causing error codes to be handled differently,
            // breaking compatibility between releases.
            //
            // [Start new internal error codes]
            //

            NatIpAddressProviderAddressRangeExhausted,
            ServiceHostTerminationInProgress,
            FabricHostServicePathNotFound,
            UpdaterServicePathNotFound,
            IsolatedNetworkInterfaceNameNotFound,
            OverlayNetworkResourceProviderAddressRangeExhausted,
            UpdateContextFailed,
            FSSPrimaryInDatalossRecovery,
            BackupCopierAborted,
            BackupCopierDisabled,
            BackupCopierRetryableError,
            ApplicationHostCrash,
            SharingAccessLockViolation,
            ApplicationDeploymentInProgress,
            //
            // [End new internal error codes]
            //
            OperationsPending,
            SecuritySessionExpiredByRemoteEnd,
            LocalResourceManagerCPUCapacityMismatch,
            LocalResourceManagerMemoryCapacityMismatch,
            NotEnoughCPUForServicePackage,
            NotEnoughMemoryForServicePackage,
            ServicePackageAlreadyRegisteredWithLRM,
            MaxResultsReached,
            JobQueueFull,
            ContainerFeatureNotEnabled,
            ContainerFailedToStart,
            ContainerCreationFailed,
            RoutingError,
            P2PError,
            NamingServiceTooBusy,
            DnsServerIPAddressNotFound,
            ContainerFailedToCreateDnsChain,
            IPAddressProviderAddressRangeExhausted,
            ApplicationPrincipalAbortableError,
            FMLocationNotKnown,
            HostingServiceTypeValidationInProgress,
            ImageBuilderAborted,
            ImageBuilderDisabled,
            InconsistentInMemoryState,
            RebootRequired,
            CertificateNotFound_DummyPlaceHolder,
            MaxFileStreamFullCopyWaiters,
            ReplicatorInternalError,
            DatabaseFilesCorrupted,
            IncomingConnectionThrottled,
            FabricRemoveConfigurationValueNotFound,
            CredentialAlreadyLoaded,
            CertificateNotMatched,
            StaleServicePackageInstance,
            CannotConnect,
            ConnectionInstanceObsolete,
            ConnectionConfirmWaitExpired,
            ConnectionIdleTimeout,
            SecurityNegotiationTimeout,
            SecuritySessionExpired,
            MessageExpired,
            ServiceNotificationFilterNotFound,
            ServiceNotificationFilterAlreadyExists,
            DuplicateMessage,
            ApplicationInstanceDeleted,
            HostingEntityAborted,
            NodeIsStopped,
            ReliableSessionConflictingSessionAborted,
            TooManyIpcDisconnect,
            CannotConnectToAnonymousTarget,
            FileStoreServiceReplicationProcessingError,
            CMRequestAborted,
            FileStoreServiceNotReady,
            StoreFatalError,
            OperationStreamFaulted,
            HostingActivationEntityNotInUse,
            StagingFileNotFound,
            FileUpdateInProgress,
            FileAlreadyExists,
            StoreTransactionNotActiveDeprecated,
            TransportSendQueueFull,
            NodeDeactivationInProgress,
            InvestigationRequired,
            HealthCheckPending,
            FabricDataRootNotFoundDeprecatedDoNotUse,
            FabricLogRootNotFound,
            FabricBinRootNotFound,
            FabricCodePathNotFound,
            StoreOperationCanceled,
            InfrastructureTaskInProgress,
            SequenceStreamRangeGapError,
            StaleInfrastructureTask,
            InfrastructureTaskNotFound,
            SystemServiceNotFound,
            HealthEntityDeleted,
            IncompatibleVersion,
            SerializationError,
            OwnerExists,
            NotOwner,
            VoteStoreAccessError,
            UnspecifiedError,
            AddressAlreadyInUse,
            NotFound,
            AlreadyExists,
            NeighborhoodLost,
            GlobalLeaseLost,
            TokenAcquireTimeout,
            RegisterWithLeaseDriverFailed,
            LeaseFailed,
            StoreUnexpectedError,
            StoreNeedsDefragment,
            StoreInUse,
            UpdatePending,
            SqlStoreUnableToInitializeCommands,
            SqlStoreRollbackTransFailed,
            SqlStoreCommitTransFailed,
            SqlTransactionInactive,
            SqlStoreTransactionAlreadyCommitted,
            SqlStoreTableNotFound,
            P2PNodeDoesNotMatchFault,
            RoutingNodeDoesNotMatchFault,
            NodeIsNotRoutingFault,
            MaxRetriesReachedFault,
            BroadcastFailed,
            MessageHandlerDoesNotExistFault,
            ConsistencyUnitNotFound,
            NameUndergoingRepair,
            RepairContradictedOperation,
            FMFailoverUnitNotFound,
            FMFailoverUnitAlreadyExists,
            FMStoreNotUsable,
            FMNotReadyForUse,
            FMStoreKeyNotFound,
            FMStoreUpdateFailed,
            FMServiceAlreadyExists,
            FMServiceDeleteInProgress,
            FMServiceDoesNotExist,
            FMApplicationUpgradeInProgress,
            FMInvalidRolloutVersion,
            CMRequestAlreadyProcessing,
            CMBusy,
            CMImageBuilderRetryableError,
            RAServiceTypeNotRegistered,
            RANotReadyForUse,
            RACouldNotCreateStoreDirectory,
            RAFailoverUnitNotFound,
            RAStoreNotUsable,
            RAStoreKeyNotFound,
            RAProxyCouldNotCreateServiceObject,
            RAProxyCouldNotOpenStatelessService,
            RAProxyCouldNotCloseStatelessService,
            RAProxyCouldNotOpenStatefulService,
            RAProxyCouldNotCloseStatefulService,
            RAProxyCouldNotChangeRoleForStatefulService,
            RAProxyDemoteCompleted,
            RAProxyBuildIdleReplicaInProgress,
            RAProxyOperationIncompatibleWithCurrentFupState,
            RAProxyUpdateReplicatorConfigurationPending,
            RAProxyStateChangedOnDataLoss,
            REDuplicateOperation,
            HostingServiceTypeRegistrationVersionMismatch,
            HostingServicePackageVersionMismatch,
            HostingApplicationVersionMismatch,
            HostingApplicationHostAlreadyRegistered,
            HostingApplicationHostNotRegistered,
            HostingFabricRuntimeAlreadyRegistered,
            HostingFabricRuntimeNotRegistered,
            HostingServiceTypeAlreadyRegisteredToSameRuntime,
            HostingServiceTypeNotOwned,
            HostingServiceTypeDisabled,
            HostingDeploymentInProgress,
            HostingActivationInProgress,
            HostingCodePackageNotHosted,
            HostingCodePackageAlreadyHosted,
            HostingDllHostNotFound,
            HostingTypeHostNotFound,
            EndpointProviderPortRangeExhausted,
            EndpointProviderNotEnabled,
            ApplicationManagerApplicationAlreadyExists,
            ApplicationManagerApplicationNotFound,
            ApplicationPrincipalDoesNotExist,
            ImageStoreInvalidStoreUri,
            ImageStoreUnableToPerformAzureOperation,
            FabricTestServiceNotOpen,
            FabricTestIncorrectServiceLocation,
            FabricTestStatusNotGranted,
            FabricTestVersionDoesNotMatch,
            FabricTestKeyDoesNotExist,
            FabricTestReplicationFailed,
            XmlInvalidContent,
            XmlUnexpectedEndOfFile,
            InvalidServiceTypeV1,
            StaleRequest,
            // 'AbandonedFileWriteLockFound' has been replaced with public error code CorruptedImageStoreObjectFound.
            // This is kept so that backward compatibility is not broken.
            AbandonedFileWriteLockFound,

            LAST_INTERNAL_ERROR_CODE = AbandonedFileWriteLockFound

            // Unlike public error codes (which must be added to the bottom of the list
            // in FabricTypes.idl), internal error codes must be added to the top
            // of this list. Do not add new error codes here.
        };

        class TextWriter;
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( ErrorCodeValue )
    }
}