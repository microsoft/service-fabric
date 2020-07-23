// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Defines error codes that can be associated with a <see cref="System.Fabric.FabricException" />.</para>
    /// </summary>
    public enum FabricErrorCode : long
    {
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that there was an unknown error.</para>
        /// </summary>
        Unknown,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the partition key is invalid.</para>
        /// </summary>
        InvalidPartitionKey = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PARTITION_KEY,

        /// <summary>
        /// <para>
        /// Indicates that certificate for user role (<see cref="System.Fabric.FabricClientRole.User" />)
        /// <see cref="System.Fabric.FabricClient" /> is not setup.
        /// </para>
        /// </summary>
        UserRoleClientCertificateNotConfigured = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the Service Fabric Name already exists.</para>
        /// </summary>
        NameAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_ALREADY_EXISTS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service already exists.</para>
        /// </summary>
        ServiceAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_ALREADY_EXISTS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service group already exists.</para>
        /// </summary>
        ServiceGroupAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application type is currently being provisioned.</para>
        /// </summary>
        ApplicationTypeProvisionInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application type already exists.</para>
        /// </summary>
        ApplicationTypeAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application already exists.</para>
        /// </summary>
        ApplicationAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_ALREADY_EXISTS,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the Service Fabric Name was not found.</para>
        /// </summary>
        NameNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_DOES_NOT_EXIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the property was not found.</para>
        /// </summary>
        PropertyNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROPERTY_DOES_NOT_EXIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the value of the property was empty.</para>
        /// </summary>
        PropertyValueEmpty = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VALUE_EMPTY,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service was not found.</para>
        /// </summary>
        ServiceNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_DOES_NOT_EXIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service group was not found.</para>
        /// </summary>
        ServiceGroupNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application type was not found.</para>
        /// </summary>
        ApplicationTypeNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application type is in use.</para>
        /// </summary>
        ApplicationTypeInUse = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_IN_USE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application does not exist.</para>
        /// </summary>
        ApplicationNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service type was not found.</para>
        /// </summary>
        ServiceTypeNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the service manifest was not found.</para>
        /// </summary>
        ServiceManifestNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_MANIFEST_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the Service Fabric Name is not empty: 
        /// there are entities such as child Names, Service or Properties associated with it.</para>
        /// </summary>
        NameNotEmpty = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_NOT_EMPTY,

        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates the node was not found.</para>
        /// </summary>
        NodeNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the node is up when it is expected to be down.</para>
        /// </summary>
        NodeIsUp = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_UP,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the replica is not the primary.</para>
        /// </summary>
        NotPrimary = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_PRIMARY,
        
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service partition does not have write quorum.</para>
        /// </summary>
        NoWriteQuorum = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NO_WRITE_QUORUM,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reconfiguration is currently in pending state.</para>
        /// </summary>
        ReconfigurationPending = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the replication queue is full.</para>
        /// </summary>
        ReplicationQueueFull = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPLICATION_QUEUE_FULL,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the replication operation is too large.</para>
        /// </summary>
        ReplicationOperationTooLarge = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPLICATION_OPERATION_TOO_LARGE,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the atomic group is invalid.</para>
        /// </summary>
        InvalidAtomicGroup = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ATOMIC_GROUP,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service is offline.</para>
        /// </summary>
        ServiceOffline = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_OFFLINE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the partition was not found.</para>
        /// </summary>
        PartitionNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PARTITION_NOT_FOUND,
        /// <summary>
        /// <para>Two <see cref="System.Fabric.ResolvedServicePartition" /> objects cannot be compared using <see cref="System.Fabric.ResolvedServicePartition.CompareVersion" /> 
        /// because they describe different replica sets.</para>
        /// </summary>
        ServiceMetadataMismatch = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_METADATA_MISMATCH,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the service affinity chain is not supported.</para>
        /// </summary>
        ServiceAffinityChainNotSupported = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED,
        
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates there was a write conflict.</para>
        /// </summary>
        WriteConflict = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_WRITE_CONFLICT,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application upgrade validation failed.</para>
        /// </summary>
        ApplicationUpgradeValidationError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that there is a mismatch in the service type.</para>
        /// </summary>
        ServiceTypeMismatch = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_MISMATCH,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service template was not found.</para>
        /// </summary>
        ServiceTemplateNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service type was already registered.</para>
        /// </summary>
        ServiceTypeAlreadyRegistered = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service type was not registered.</para>
        /// </summary>
        ServiceTypeNotRegistered = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_NOT_REGISTERED,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the application is currently being upgraded.</para>
        /// </summary>
        ApplicationUpgradeInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS,

        /// <summary>
        /// A FabricErrorCode that indicates the application is currently being updated.
        /// </summary>
        ApplicationUpdateInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS,

        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the upgrade domain was already completed.</para>
        /// </summary>
        UpgradeDomainAlreadyCompleted = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED,

        /// <summary>
        /// <para>The specified code or Cluster Manifest version cannot be unprovisioned or used as the target of an upgrade because it has not been provisioned.</para>
        /// </summary>
        FabricVersionNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_NOT_FOUND,
        /// <summary>
        /// <para>The specified code or Cluster Manifest version cannot be unprovisioned because it is either being used by the cluster or is the target of a cluster upgrade.</para>
        /// </summary>
        FabricVersionInUse = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_IN_USE,
        /// <summary>
        /// <para>The specified code or Cluster Manifest version has already been provisioned in the system.</para>
        /// </summary>
        FabricVersionAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS,
        /// <summary>
        /// <para>The Service Fabric cluster is already in the target code or Cluster Manifest version specified by the upgrade request.</para>
        /// </summary>
        FabricAlreadyInTargetVersion = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION,
        /// <summary>
        /// <para>The Service Fabric cluster is not currently being upgrade and the request is only valid during upgrade.</para>
        /// </summary>
        FabricNotUpgrading = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_NOT_UPGRADING,
        /// <summary>
        /// <para>The Service Fabric Cluster is currently begin upgraded and the request is not valid during upgrade.</para>
        /// </summary>
        FabricUpgradeInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS,
        /// <summary>
        /// <para>An error in the Service Fabric cluster upgrade request was discovered during pre-upgrade validation of the upgrade description and Cluster Manifest.</para>
        /// </summary>
        FabricUpgradeValidationError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the <see cref="System.Fabric.FabricClient.HealthClient" /> has reached the maximum number of health reports that can accept for processing. More reports will be accepted when progress is done with the currently accepted reports. By default, the <see cref="System.Fabric.FabricClient.HealthClient" /> can accept 10000 different reports.</para>
        /// </summary>
        FabricHealthMaxReportsReached = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_HEALTH_MAX_REPORTS_REACHED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the report is stale. Returned when <see cref="System.Fabric.FabricClient.HealthClient" /> 
        /// has a <see cref="System.Fabric.Health.HealthReport" /> for the same entity, <see cref="System.Fabric.Health.HealthInformation.SourceId" /> 
        /// and <see cref="System.Fabric.Health.HealthInformation.Property" /> with same or higher <see cref="System.Fabric.Health.HealthInformation.SequenceNumber" />.</para>
        /// </summary>
        FabricHealthStaleReport = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_HEALTH_STALE_REPORT,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the health entity is not found in the Health Store. Returned when Health Store has no reports from a Service Fabric system component on the entity or on one of the hierarchical parents. This can happen if the entity or one of its parents doesn�t exist in the Service Fabric cluster, or the reports didn�t yet arrive at the health store.</para>
        /// </summary>
        FabricHealthEntityNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_HEALTH_ENTITY_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates Service Fabric service is too busy to accept requests at this time. This is a transient error.</para>
        /// </summary>
        ServiceTooBusy = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TOO_BUSY,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates a communication error has occurred.</para>
        /// </summary>
        CommunicationError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMMUNICATION_ERROR,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the gateway could not be reached. This is a transient error.</para>
        /// </summary>
        GatewayNotReachable = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_GATEWAY_NOT_REACHABLE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the object was closed.</para>
        /// </summary>
        ObjectClosed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OBJECT_CLOSED,

        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates the a Check <see cref="System.Fabric.PropertyBatchOperation" /> has failed.</para>
        /// </summary>
        PropertyCheckFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROPERTY_CHECK_FAILED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the enumeration completed.</para>
        /// </summary>
        EnumerationCompleted = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ENUMERATION_COMPLETED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the configuration section was not found.</para>
        /// </summary>
        ConfigurationSectionNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the configuration parameter was not found.</para>
        /// </summary>
        ConfigurationParameterNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the configuration was invalid.</para>
        /// </summary>
        InvalidConfiguration = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CONFIGURATION,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the image builder validation error as occurred.</para>
        /// </summary>
        ImageBuilderValidationError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the path passed by the user starts with a reserved directory.</para>
        /// </summary>
        ImageBuilderReservedDirectoryError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR,      
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the replica does not exist.</para>
        /// </summary>
        ReplicaDoesNotExist = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPLICA_DOES_NOT_EXIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the process got deactivated.</para>
        /// </summary>
        ProcessDeactivated = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROCESS_DEACTIVATED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the process aborted.</para>
        /// </summary>
        ProcessAborted = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROCESS_ABORTED,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the address was invalid.</para>
        /// </summary>
        InvalidAddress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ADDRESS,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates the URI is invalid.</para>
        /// </summary>
        InvalidNameUri = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_NAME_URI,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the property value is too large.</para>
        /// </summary>
        ValueTooLarge = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VALUE_TOO_LARGE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the directory was not found.</para>
        /// </summary>
        DirectoryNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DIRECTORY_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the path is too long.</para>
        /// </summary>
        PathTooLong = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PATH_TOO_LONG,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the file was not found.</para>
        /// </summary>
        FileNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FILE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the code is not ready.</para>
        /// </summary>
        NotReady = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the operation timed out.</para>
        /// </summary>
        OperationTimedOut = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the operation did not completed.</para>
        /// </summary>
        OperationNotComplete = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OPERATION_NOT_COMPLETE,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the code package was not found.</para>
        /// </summary>
        CodePackageNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CODE_PACKAGE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that configuration package was not found.</para>
        /// </summary>
        ConfigurationPackageNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the data package was not found.</para>
        /// </summary>
        DataPackageNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DATA_PACKAGE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the endpoint resource was not found.</para>
        /// </summary>
        EndpointResourceNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the credential type is invalid.</para>
        /// </summary>
        InvalidCredentialType = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CREDENTIAL_TYPE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the X509FindType is invalid.</para>
        /// </summary>
        InvalidX509FindType = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_FIND_TYPE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the X509 store location is invalid.</para>
        /// </summary>
        InvalidX509StoreLocation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE_LOCATION,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates that the X509 store name is invalid.</para>
        /// </summary>
        InvalidX509StoreName = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE_NAME,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the X509 certificate thumbprint string is invalid.</para>
        /// </summary>
        InvalidX509Thumbprint = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_THUMBPRINT,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates the protection level is invalid.</para>
        /// </summary>
        InvalidProtectionLevel = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PROTECTION_LEVEL,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the X509 certificate store cannot be opened.</para>
        /// </summary>
        InvalidX509Store = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates the subject name is invalid.</para>
        /// </summary>
        InvalidSubjectName = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_SUBJECT_NAME,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the format of common name list string is invalid. It should be a comma separated list</para>
        /// </summary>
        InvalidAllowedCommonNameList = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the credentials are invalid.</para>
        /// </summary>
        InvalidCredentials = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CREDENTIALS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the decryption failed.</para>
        /// </summary>
        DecryptionFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DECRYPTION_FAILED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the encryption has failed.</para>
        /// </summary>
        EncryptionFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ENCRYPTION_FAILED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the image store object was corrupted.</para>
        /// </summary>
        CorruptedImageStoreObjectFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the ImageBuilder hit an unexpected error.</para>
        /// </summary>
        ImageBuilderUnexpectedError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the ImageBuilder was not able to perform the operation in the specified timeout.</para>
        /// </summary>
        ImageBuilderTimeoutError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_TIMEOUT,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the ImageBuilder hit an AccessDeniedException when using the ImageStore.</para>
        /// </summary>
        ImageBuilderAccessDeniedError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_ACCESS_DENIED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the MSI file being provisioned is not valid.</para>
        /// </summary>
        ImageBuilderInvalidMsiFile = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates there was an ImageStoreIOEception.</para>
        /// </summary>
        ImageStoreIOException = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGESTORE_IOERROR,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates that the operation failed to acquire a lock.
        /// </para>
        /// </summary>
        ImageStoreAcquireFileLockFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ACQUIRE_FILE_LOCK_FAILED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the ServiceType was not defined in the service manifest.</para>
        /// </summary>
        InvalidServiceType = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_SERVICE_TYPE,

        /// <summary>
        /// <para>The application is not currently being upgraded and the request is only valid during upgrade.</para>
        /// </summary>
        ApplicationNotUpgrading = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_UPGRADING,
        /// <summary>
        /// <para>The application is already in the target version specified by an application upgrade request.</para>
        /// </summary>
        ApplicationAlreadyInTargetVersion = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the key is too large.</para>
        /// </summary>
        KeyTooLarge = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_TOO_LARGE,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the key cannot be found.</para>
        /// </summary>
        KeyNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the sequence number check failed. This usually happens when there is a conflicting operation executed on the same object which modifies the sequence number.</para>
        /// </summary>
        SequenceNumberCheckFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the transaction is not active because it has already been committed or rolled back.</para>
        /// </summary>
        TransactionNotActive = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TRANSACTION_NOT_ACTIVE,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the transaction is too large, which typically results when the transaction either contains too many operations or the size of the data being written is too large.</para>
        /// </summary>
        TransactionTooLarge = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TRANSACTION_TOO_LARGE,

        /// <summary>
        /// <para>
        /// FabricErrorCode that indicates that one transaction can't be used by multiple threads simultaneously.
        /// </para>
        /// </summary>
        MultithreadedTransactionsNotAllowed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED,

        /// <summary>
        /// FabricErrorCode that indicates that the transaction was aborted.
        /// </summary>
        TransactionAborted = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TRANSACTION_ABORTED,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session transport startup has failed.</para>
        /// </summary>
        ReliableSessionTransportStartupFailure = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session already exists.</para>
        /// </summary>
        ReliableSessionAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session cannot connect.</para>
        /// </summary>
        ReliableSessionCannotConnect = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT,
        /// <summary>
        /// <para>A   <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session manager already exists.</para>
        /// </summary>
        ReliableSessionManagerExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session was rejected.</para>
        /// </summary>
        ReliableSessionRejected = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_REJECTED,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session cannot be found.</para>
        /// </summary>
        ReliableSessionNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_NOT_FOUND,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session queue is empty.</para>
        /// </summary>
        ReliableSessionQueueEmpty = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session quota exceeded.</para>
        /// </summary>
        ReliableSessionQuotaExceeded = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session service was faulted.</para>
        /// </summary>
        ReliableSessionServiceFaulted = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session manager is already listening.</para>
        /// </summary>
        ReliableSessionManagerAlreadyListening = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session manager was not found.</para>
        /// </summary>
        ReliableSessionManagerNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND,
        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session manager is not listening.</para>
        /// </summary>
        ReliableSessionManagerNotListening = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the reliable session has invalid target partition.</para>
        /// </summary>
        ReliableSessionInvalidTargetPartition = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION,

        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the repair task already exists.</para>
        /// </summary>
        RepairTaskAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPAIR_TASK_ALREADY_EXISTS,
        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the repair task was not found.</para>
        /// </summary>
        RepairTaskNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPAIR_TASK_NOT_FOUND,

        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the instance identifier doesn�t match.</para>
        /// </summary>
        InstanceIdMismatch = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INSTANCE_ID_MISMATCH,

        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the node has not stopped yet.</para>
        /// </summary>
        NodeHasNotStoppedYet = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_HAS_NOT_STOPPED_YET,

        /// <summary>
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode" /> that indicates that the cluster capacity is insufficient.</para>
        /// </summary>
        InsufficientClusterCapacity = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the specified constraint key is undefined.
        /// </para>
        /// </summary>
        ConstraintKeyUndefined = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONSTRAINT_KEY_UNDEFINED,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the package sharing policy is incorrect.
        /// </para>
        /// </summary>
        InvalidPackageSharingPolicy = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PACKAGE_SHARING_POLICY,

        /// <summary>
        /// <para>
        /// Predeployed of application package on Node not allowed. Predeployment feature requires ImageCache to be enabled on node.
        /// </para>
        /// </summary>
        PreDeploymentNotAllowed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED,

        /// <summary>
        /// <para>
        /// Invalid backup setting. E.g. incremental backup option is not set upfront etc.
        /// </para>
        /// </summary>
        InvalidBackupSetting = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_BACKUP_SETTING,

        /// <summary>
        /// <para>
        /// Cannot restart a replica of a volatile stateful service or an instance of a stateless service
        /// </para>
        /// </summary>
        InvalidReplicaOperation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_REPLICA_OPERATION,

        /// <summary>
        /// <para>
        /// The replica is currently transitioning (closing or opening) and the operation cannot be performed at this time
        /// </para>
        /// </summary>
        InvalidReplicaStateForReplicaOperation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_REPLICA_STATE,

        /// <summary>
        /// <para>
        /// Incremental backups can only be done after an initial full backup.
        /// </para>
        /// </summary>
        MissingFullBackup = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_MISSING_FULL_BACKUP,

        /// <summary>
        /// <para>
        /// A backup is currently in progress.
        /// </para>
        /// </summary>
        BackupInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_IN_PROGRESS,

        /// <summary>
        /// <para>
        /// The Cluster Resource Balancer is not yet ready to handle the operation.
        /// </para>
        /// </summary>
        PLBNotReady = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_LOADBALANCER_NOT_READY,
        /// <summary>
        /// <para>Indicates that a service notification filter has already been registered at the specified name by the current Fabric Client.</para>
        /// </summary>
        DuplicateServiceNotificationFilterName = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME,

        /// <summary>
        /// A FabricErrorCode that indicates the partition operation is invalid.
        /// </summary>
        InvalidPartitionOperation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PARTITION_OPERATION,

        /// <summary>
        /// The replica already has Primary role.
        /// </summary>
        AlreadyPrimaryReplica = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PRIMARY_ALREADY_EXISTS,

        /// <summary>
        /// The replica already has Secondary role.
        /// </summary>
        AlreadySecondaryReplica = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SECONDARY_ALREADY_EXISTS,

        /// <summary>
        /// <para>
        /// The backup directory is not empty.
        /// </para>
        /// </summary>
        BackupDirectoryNotEmpty = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY,

        /// <summary>
        /// <para>
        /// The replica belongs to a self-activated service. The ForceRemove option is not supported for such replicas
        /// </para>
        /// </summary>
        ForceNotSupportedForReplicaControlOperation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the connection was denied by the remote side.
        /// </para>
        /// </summary>
        ConnectionDenied = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONNECTION_DENIED,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the authentication failed.
        /// </para>
        /// </summary>
        ServerAuthenticationFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVER_AUTHENTICATION_FAILED,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates there was a connection failure.
        /// </para>
        /// </summary>
        FabricCannotConnect = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CANNOT_CONNECT,

        /// <summary>
        /// A FabricErrorCode that indicates the connection was closed by the remote end.
        /// </summary>
        FabricConnectionClosedByRemoteEnd = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the message is too large.
        /// </para>
        /// </summary>
        FabricMessageTooLarge = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_MESSAGE_TOO_LARGE,

        /// <summary>
        /// <para>
        /// The service's and cluster's configuration settings would result in a constraint-violating state if the operation were executed.
        /// </para>
        /// </summary>
        ConstraintNotSatisfied = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONSTRAINT_NOT_SATISFIED,

        /// <summary>
        /// <para>
        /// A FabricErrorCode that indicates the specified endpoint was not found.
        /// </para>
        /// </summary>
        FabricEndpointNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ENDPOINT_NOT_FOUND,

        /// <summary>
        /// A FabricErrorCode that indicates that an object appears more than once in an array of synchronization objects.
        /// </summary>
        DuplicateWaitObject = NativeTypes.FABRIC_ERROR_CODE.COR_E_DUPLICATEWAITOBJECT,

        /// <summary>
        /// A FabricErrorCode that indicates that the entry point was not found. This happens when type loading failures occur.
        /// </summary>
        EntryPointNotFound = NativeTypes.FABRIC_ERROR_CODE.COR_E_TYPELOAD,
        
        /// <summary>
        /// Deletion of backup files/directory failed. Currently this can happen
        /// in a scenario where backup is used mainly to truncate logs.
        /// </summary>
        DeleteBackupFileFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DELETE_BACKUP_FILE_FAILED,

        /// <summary>
        /// Operation was canceled by the system and should be retried by the client.
        /// </summary>
        OperationCanceled = NativeTypes.FABRIC_ERROR_CODE.E_ABORT,

        /// <summary>
        /// A FabricErrorCode that indicates that this API call is not valid for the current state of the test command.
        /// </summary>
        InvalidTestCommandState = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_TEST_COMMAND_STATE,

        /// <summary>
        /// A FabricErrorCode that indicates that this test command operation id (Guid) is already being used.
        /// </summary>
        TestCommandOperationIdAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS,

        /// <summary>
        /// A FabricErrorCode that indicates that an instance of Chaos is already running.
        /// </summary>
        ChaosAlreadyRunning = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CHAOS_ALREADY_RUNNING,

        /// <summary>
        /// Creation or deletion terminated due to persistent failures after bounded retry.
        /// </summary>
        CMOperationFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CM_OPERATION_FAILED,
        
        /// <summary>
        /// Fabric Data Root is not defined on the target machine.
        /// </summary>
        FabricDataRootNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_DATA_ROOT_NOT_FOUND,

        /// <summary>
        /// Indicates that restore metadata present in supplied restore directory in invalid.
        /// </summary>
        InvalidRestoreData = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_RESTORE_DATA,

        /// <summary>
        /// Indicates that backup-chain in specified restore directory contains duplicate backups.
        /// </summary>
        DuplicateBackups = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DUPLICATE_BACKUPS,

        /// <summary>
        /// Indicates that backup-chain in specified restore directory has one or more missing backups.
        /// </summary>
        InvalidBackupChain = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_BACKUP_CHAIN,

        /// <summary>
        /// An operation is already in progress.
        /// </summary>
        StopInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_STOP_IN_PROGRESS,

        /// <summary>
        /// The node is already in a stopped state
        /// </summary>
        AlreadyStopped = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ALREADY_STOPPED,

        /// <summary>
        /// The node is down (not stopped).
        /// </summary>
        NodeIsDown = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_IS_DOWN,

        /// <summary>
        /// Node transition in progress
        /// </summary>
        NodeTransitionInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_TRANSITION_IN_PROGRESS,

        /// <summary>
        /// The provided instance id is invalid.
        /// </summary>
        InvalidInstanceId = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_INSTANCE_ID,

        /// <summary>
        /// The provided duration is invalid.
        /// </summary>
        InvalidDuration = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_DURATION,

        /// <summary>
        /// Indicates that backup provided for restore is invalid.
        /// </summary>
        InvalidBackup = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_BACKUP,

        /// <summary>
        /// Indicates that backup provided for restore has older data than present in service.
        /// </summary>
        RestoreSafeCheckFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RESTORE_SAFE_CHECK_FAILED,

        /// <summary>
        /// Indicates that the config upgrade fails.
        /// </summary>
        ConfigUpgradeFailed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIG_UPGRADE_FAILED,

        /// <summary>
        /// Indicates that the upload session range will overlap or are out of range.
        /// </summary>
        UploadSessionRangeNotSatisfiable = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE,

        /// <summary>
        /// Indicates that the upload session ID is existed for a different image store relative path.
        /// </summary>
        UploadSessionIdConflict = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_UPLOAD_SESSION_ID_CONFLICT,

        /// <summary>
        /// Indicates that the partition selector is invalid.
        /// </summary>
        InvalidPartitionSelector = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PARTITION_SELECTOR,

        /// <summary>
        /// Indicates that the replica selector is invalid.
        /// </summary>
        InvalidReplicaSelector = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_REPLICA_SELECTOR,

        /// <summary>
        /// Indicates that DnsService is not enabled on the cluster.
        /// </summary>
        DnsServiceNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DNS_SERVICE_NOT_FOUND,

        /// <summary>
        /// Indicates that service DNS name is invalid.
        /// </summary>
        InvalidDnsName = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_DNS_NAME,

        /// <summary>
        /// Indicates that service DNS name is in use by another service.
        /// </summary>
        DnsNameInUse = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DNS_NAME_IN_USE,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the compose deployment already exists.</para>
        /// </summary>
        ComposeDeploymentAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the compose deployment is not found.</para>
        /// </summary>
        ComposeDeploymentNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the operation is only valid for stateless services.</para>
        /// </summary>
        InvalidForStatefulServices = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_FOR_STATEFUL_SERVICES,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the operation is only valid for stateful services.</para>
        /// </summary>
        InvalidForStatelessServices = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_FOR_STATELESS_SERVICES,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the operation is only valid for stateful persistent services.</para>
        /// </summary>
        OnlyValidForStatefulPersistentServices = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the upload session ID is invalid. Plesea use GUID as upload session ID.</para>
        /// </summary>
        InvalidUploadSessionId = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_UPLOAD_SESSION_ID,

        /// <summary>
        /// Indicates that the backup protection is not enabled
        /// </summary>
        BackupNotEnabled = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_NOT_ENABLED,

        /// <summary>
        /// Indicates that there is a backup protection enablement
        /// </summary>
        BackupEnabled = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_IS_ENABLED,

        /// <summary>
        /// Indicates the Backup Policy does not Exist
        /// </summary>
        BackupPolicyDoesNotExist = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST,

        /// <summary>
        /// Indicates the Backup Policy is already Exists
        /// </summary>
        BackupPolicyAlreayExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS,

        /// <summary>
        /// Indicates that a partition is already has a restore in progress
        /// </summary>
        RestoreAlreadyInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RESTORE_IN_PROGRESS,

        /// <summary>
        /// Indicates that the source from where restore is requested has a properties mismatch with target partition
        /// </summary>
        RestoreSourceTargetPartitionMismatch = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH,

        /// <summary>
        /// Indicates the Restore cannot be triggered as Fault Analysis Service is not running
        /// </summary>
        FaultAnalysisServiceNotEnabled =  NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the container is not found.</para>
        /// </summary>
        ContainerNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONTAINER_NOT_FOUND,

        /// <summary>
        /// Indicates that the operation is performed on a disposed object.
        /// </summary>
        ObjectDisposed = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OBJECT_DISPOSED,

        /// <summary>
        /// Indicates the partition is not readable.
        /// </summary>
        NotReadable = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READABLE,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that operation is invalid.</para>
        /// </summary>
        InvalidOperation = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_OPERATION,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the single instance application already exists.</para>
        /// </summary>
        SingleInstanceApplicationAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the single instance application is not found.</para>
        /// </summary>
        SingleInstanceApplicationNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the volume already exists.</para>
        /// </summary>
        VolumeAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VOLUME_ALREADY_EXISTS,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the volume is not found.</para>
        /// </summary>
        VolumeNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VOLUME_NOT_FOUND,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the scaling policy specified for the service is invalid. 
        /// Verify that every <see cref="System.Fabric.Description.ScalingTriggerDescription" /> and <see cref="System.Fabric.Description.ScalingMechanismDescription" /> is valid in the context of the kind and metrics of the service.</para>
        /// </summary>
        InvalidServiceScalingPolicy = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_SERVICE_SCALING_POLICY,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the service is undergoing database migration and writes are currently not available.</para>
        /// </summary>
        DatabaseMigrationInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DATABASE_MIGRATION_IN_PROGRESS,
        
        /// <summary>
        /// Indicates generic error happens in central secret service
        /// </summary>
        CentralSecretServiceGenericError = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC,

        /// <summary>
        /// <para>A <see cref="System.Fabric.FabricErrorCode" /> that indicates the compose deployment is not upgrading. Call <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.GetComposeDeploymentUpgradeProgressAsync(string)" /> to get more information.</para>
        /// </summary>
        ComposeDeploymentNotUpgrading = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING,
		
        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the secret is invalid
        /// </summary>
        SecretInvalid = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SECRET_INVALID,

        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the secret version already exists
        /// </summary>
        SecretVersionAlreadyExists = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SECRET_VERSION_ALREADY_EXISTS,

        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the single instance application's upgrade is in progress.
        /// </summary>
        SingleInstanceApplicationUpgradeInProgress = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS,

        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the operation is not supported.
        /// </summary>
        OperationNotSupported = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OPERATION_NOT_SUPPORTED,

        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the network is not found.
        /// </summary>
        NetworkNotFound = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NETWORK_NOT_FOUND,

        /// <summary>
        /// A <see cref="System.Fabric.FabricErrorCode" /> that indicates that the network is currently in use.
        /// </summary>
        NetworkInUse = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NETWORK_IN_USE
    }
}