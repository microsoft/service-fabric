// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum ClusterManagementErrorCode
    {
        /// <summary>
        /// An error occurred.
        /// </summary>
        InternalServerError,

        /// <summary>
        /// Operation was canceled.
        /// </summary>
        Canceled,

        /// <summary>
        /// Resource {0} not found.
        /// </summary>
        NotFound,

        /// <summary>
        /// There was a conflict when modifying key {0}.
        /// </summary>
        WriteConflict,

        /// <summary>
        /// Request is invalid.
        /// </summary>
        BadRequest,

        /// <summary>
        /// A retriable error occurred.
        /// </summary>
        RetryableError,

        /// <summary>
        /// API version query parameter is not provided.
        /// </summary>
        ApiVersionMissing,

        /// <summary>
        /// Invalid API version specified in the request url.
        /// </summary>
        InvalidApiVersion,

        /// <summary>
        /// ETAG provided in if-match header {0} does not match ETAG in the store {1}.
        /// </summary>
        PreconditionFailed,

        /// <summary>
        /// Cannot parse the request.
        /// </summary>
        /// <remarks>
        /// This is a top-level error with invalid JSON, InvalidJSONReferenceWrongType used as error details.
        /// </remarks>
        InvalidRequestFormat,

        /// <summary>
        /// Errors encountered while parsing the request body.
        /// </summary>
        InvalidJson,

        /// <summary>
        /// {0} operation on resource {1} is not supported.
        /// </summary>
        OperationNotSupported,

        /// <summary>
        /// Unauthorized operation.
        /// </summary>
        UnauthorizedOperation,

        /// <summary>
        /// Resource name {0} is invalid. The name can be up to 80 characters long. It must begin with a word character, and it must end with a word character or with '_'. The name may contain word characters or '.', '-', '_'.
        /// </summary>
        InvalidResourceName,

        /// <summary>
        /// Cluster size: {0} is not supported. Total node count needs to be either one node for testing/demo purpose or at least {1} and maximum {2}.
        /// </summary>
        ClusterSizeNotSupported,

        /// <summary>
        /// Each machine can only host one node.
        /// </summary>
        ScaleMinNotAllowed,

        /// <summary>
        /// Fault domain count must be greater than 2, current FD count {0}.
        /// </summary>
        FDMustBeGreaterThan2,

        /// <summary>
        /// Upgrade domain count must be greater than 2, current UD count {0}.
        /// </summary>
        UDMustBeGreaterThan2,

        /// <summary>
        /// Cluster size '{0}' is not supported. Cluster size should be either 1 or 5.
        /// </summary>
        DevClusterSizeNotSupported,

        /// <summary>
        /// Role '{0}' has NodeTypeRef '{1}'. This NodeType is not defined.
        /// </summary>
        NodeTypeNotDefined,

        /// <summary>
        /// NodeTypes cannot be null. At least one NodeType should be defined.
        /// </summary>
        NodeTypesCannotBeNull,

        /// <summary>
        /// NodeType '{0}' is marked as primary, but there is no matching VM resource referencing this NodeType. Primary NodeType should have a matching VM resource.
        /// </summary>
        PrimaryNodeTypeShouldHaveMatchingVMResource,

        /// <summary>
        /// Primary NodeType '{0}' has only {1} VM instances. At least {2} VM instances required for {3} reliability level.
        /// </summary>
        VMInstanceCountNotSufficientForReliability,

        /// <summary>
        /// Section '{0}' is required.
        /// </summary>
        RequiredSectionNotFound,
        
        /// <summary>
        /// For a multi-machine cluster, DCA store can only be a file share. The current DCA store type is {0}.
        /// </summary>
        MultiMachineClusterHasToUseFileShare,

        /// <summary>
        /// Node name {0} contains invalid characters.
        /// </summary>
        NodeNameContainsInvalidChars,

        /// <summary>
        /// Cluster name {0} contains invalid characters.
        /// </summary>
        ClusterNameContainsInvalidChars,

        /// <summary>
        /// The IP address {0} of node {1} is Malformed.
        /// </summary>
        MalformedIPAddress,

        /// <summary>
        /// Section '{0}' and Parameter '{1}' is required.
        /// </summary>
        RequiredParameterNotFound,

        /// <summary>
        /// Section '{0}' is not allowed.
        /// </summary>
        SectionNotAllowed,

        /// <summary>
        /// Section '{0}' and Parameter '{1}' is not allowed.
        /// </summary>
        ParameterNotAllowed,

        /// <summary>
        /// Endpoint '{0}' is not specified in NodeType '{1}'
        /// </summary>
        EndpointNotSpecified,

        /// <summary>
        /// Application Port Count '{0}' in NodeType '{1}' is invalid. The value should be between {2} and {3}.
        /// </summary>
        ApplicationPortCountOutOfRange,

        /// <summary>
        /// Resource {0} is not ready.
        /// </summary>
        ResourceNotReady,

        /// <summary>
        /// They MSI file for version '{0}' was successfully downloaded but is invalid.
        /// </summary>
        InvalidMsi,

        /// <summary>
        /// MSI file for version '{0}' is not found.
        /// </summary>
        MsiFileNotFound,

        /// <summary>
        /// Configurations metadata file for version '{0}' is not found.
        /// </summary>
        ConfigurationMetadataFileNotFound,

        /// <summary>
        /// '{0}' has invalid port value of '{1}'. The value should be between {2} and {3}.
        /// </summary>
        PortOutOfRange,

        /// <summary>
        /// '{0}' port value of '{1}' falls within Service Fabric reserved port range. Port value should be outside '{2}' and '{3}'.
        /// </summary>
        PortInReservedRange,

        /// <summary>
        /// The StartPort of '{0}' is greater than EndPort.
        /// </summary>
        StartPortGreaterThanEnd,

        /// <summary>
        /// The DynamicPorts provided is insufficient. There should be at least {0} ports.
        /// </summary>
        InsufficientDynamicPorts,

        /// <summary>
        /// Invalid Partition Load Distribution Weight - (valid values 0 to 1).
        /// </summary>
        InvalidLdw,

        /// <summary>
        /// The Http gateway endpoint protocol '{0}' is not valid. Valid values are '{1}' and '{2}'.
        /// </summary>
        InvalidHttpGatewayProtocol,

        /// <summary>
        /// The client connection endpoint protocol '{0}' is not valid. Valid value is '{1}'.
        /// </summary>
        InvalidClientConnectionProtocol,

        /// <summary>
        /// The update of '{0}' is not allowed.
        /// </summary>
        UpdateNotAllowed,

        /// <summary>
        /// The update of the cluster is currently unavailable. Please ensure that the cluster is up and system services are healthy.
        /// </summary>
        ClusterUpdateUnavailable,

        /// <summary>
        /// Subscription {0} is not registered with the RP.
        /// </summary>
        SubscriptionNotRegistered,

        /// <summary>
        /// Client certificates cannot be defined when cluster certificate is not defined.
        /// </summary>
        ClientCertDefinedWithoutServerCert,

        /// <summary>
        /// Client common names cannot be defined without issuer thumbprints
        /// </summary>
        ClientCommonNameDefinedWithoutIssuerThumbprint,

        /// <summary>
        /// Certificate {0} is not installed under {1}. Check the node availability, certificate installation, and the remote certificate store read permission on the nodes.
        /// </summary>
        CertificateNotInstalled,

        /// <summary>
        /// Certificate {0} is not installed on node {1} under {2}. Check the node availability, certificate installation, and the remote certificate store read permission on the nodes.
        /// </summary>
        CertificateNotInstalledOnNode,

        /// <summary>
        /// Certificate {0} is either not effective yet, or expired.
        /// </summary>
        CertificateInvalid,

        /// <summary>
        /// Upgrading from 2 different certificates to 2 different certificates is not allowed.
        /// </summary>
        TwoCertificatesToTwoCertificatesNotAllowed,

        /// <summary>
        /// Upgrading from 1 certificate to 2 certificates or 2 to 1 is not allowed if there is no thumbprint or common name intersection.
        /// </summary>
        CertificateUpgradeWithNoIntersectionNotAllowed,

        /// <summary>
        /// No diagnostics storage account is available to be assigned to subscription {0}.
        /// </summary>
        DiagnosticsStorageAccountOutOfInventory,

        /// <summary>
        /// Subscription {0} is not permitted to use resources in this region.
        /// </summary>
        SubscriptionNotAllowed,

        /// <summary>
        /// Resource {0} cannot be updated since it is marked to be moved.
        /// </summary>
        MovingResourceCannotBeUpdated,

        /// <summary>
        /// The resource {0} is locked as a move is in progress.
        /// </summary>
        ResourceIsLocked,

        /// <summary>
        /// Version {0} is not found.
        /// </summary>
        VersionNotFound,

        /// <summary>
        /// The server timed out waiting for the request.
        /// </summary>
        RequestTimeout,

        /// <summary>
        /// The new certificate cannot be added since the authentication type of the cluster cannot be modified.
        /// </summary>
        AuthenticationTypeCannotBeChangedByAddingCertificate,

        /// <summary>
        /// The certificate cannot be removed since the authentication type of the cluster cannot be modified.
        /// </summary>
        AuthenticationTypeCannotBeChangedByRemovingCertificate,

        /// <summary>
        /// Authentication type cannot be changed from Windows to unsecured.
        /// </summary>
        AuthenticationTypeCannotBeChangedFromWindowsToUnsecured,

        /// <summary>
        /// Authentication type cannot be changed from unsecured to Windows.
        /// </summary>
        AuthenticationTypeCannotBeChangedFromUnsecuredToWindows,

        /// <summary>
        /// Certificates have to be rolled over one at a time. All existing certificates cannot be modified at the same time.
        /// </summary>
        CertificateUpdateNotRolling,

        /// <summary>
        /// Certificates cannot be added and removed at the same time. Remove a certificate first before adding a new one.
        /// </summary>
        AddAndRemoveCertificateNotAllowed,

        /// <summary>
        /// The cluster is going through a an upgrade which cannot be interrupted.
        /// </summary>
        PendingClusterUpgradeCannotBeInterrupted,

        /// <summary>
        /// The certificate upgrade and scale upgrade are not allowed at same time. Please try one at a time.
        /// </summary>
        CertificateAndScaleUpgradeTogetherNotAllowed,

        /// <summary>
        /// The scale up upgrade and scale down upgrade are not allowed for this cluster resource. Please create new cluster.
        /// </summary>
        ScaleUpAndScaleDownUpgradeNotAllowedForOlderClusters,

        /// <summary>
        /// The FabricSettings with SectionName '{0}' and ParameterName '{1}' is not valid.
        /// </summary>
        InvalidFabricSettingsParameter,

        /// <summary>
        /// NodeTypeRef '{0}' is not recognized for resource {1}.
        /// </summary>
        NodeTypeRefNotRecognized,

        /// <summary>
        /// NodeType '{0}' is specified more than once. Duplicate NodeTypes are not allowed.
        /// </summary>
        DuplicateNodeTypeNotAllowed,

        /// <summary>
        /// There should be exactly one NodeType marked as primary. Currently there are {0} primary NodeTypes.
        /// </summary>
        InvalidNumberofPrimaryNodeTypes,

        /// <summary>
        /// NodeType '{0}' is referenced by more than one compute resource. There should be an one to one mapping between NodeTypes and compute resources.
        /// </summary>
        DuplicateNodeTypeRefNotAllowed,

        /// <summary>
        /// NodeType '{0}' is a primary NodeType and cannot be deleted.
        /// </summary>
        PrimaryNodeTypeDeletionNotAllowed,

        /// <summary>
        /// NodeType '{0}' is set as the primary NodeType. Primary NodeType cannot be modified once cluster is created.
        /// </summary>
        PrimaryNodeTypeModificationNotAllowed,

        /// <summary>
        /// VMInstance count cannot be modified on cluster resource.
        /// </summary>
        ManualScaleUpOrDownNotAllowed,

        /// <summary>
        /// Cluster cannot have a combination of VMSS and non-VMSS compute resource.
        /// </summary>
        MixedVMResourceTypesNotAllowed,

        /// <summary>
        /// Encryption is not supported for file share diagnostics store type.
        /// </summary>
        EncryptionForFileShareStoreTypeNotSupported,

        /// <summary>
        /// Unprotected account keys for diagnostics storage accounts are not supported. Please provide diagnostics storage account keys via ProtectedAccountKeyName in DiagnosticsStorageAccountConfig.
        /// It can be StorageAccountKey1 or StorageAccountKey2 which refers to the protected settings of Service Fabric VM extensions with the same names where the real account keys are provided.
        /// </summary>
        UnprotectedDiagnosticsStorageAccountKeysNotAllowed,

        /// <summary>
        /// Please provide diagnostics storage account keys via ProtectedAccountKeyName in DiagnosticsStorageAccountConfig.
        /// It can be StorageAccountKey1 or StorageAccountKey2 which refers to the protected settings of Service Fabric VM extensions
        /// with the same names where the real account keys are provided.
        /// </summary>
        InvalidProtectedDiagnosticsStorageAccountKeyName,

        /// <summary>
        /// Please provide either protected account key (protectedAccountKeyName) or unprotected account keys (primaryAccessKey and secondaryAccessKey)
        /// in cluster resource property diagnosticsStorageAccountConfig, but not both. ProtectedAccountKeyName can be StorageAccountKey1 or StorageAccountKey2
        /// which refers to the protected settings of Service Fabric VM extensions with the same names where the real account keys are provided.
        /// </summary>
        ProtectedAndUnprotectedDiagnosticsStorageAccountKeysNotCompatible,

        /// <summary>
        /// Please provide diagnostics storage account keys via ProtectedAccountKeyName in DiagnosticsStorageAccountConfig.
        /// It can be StorageAccountKey1 or StorageAccountKey2 which refers to the protected settings of Service Fabric VM extensions
        /// with the same names where the real account keys are provided.
        /// </summary>
        ProtectedDiagnosticsStorageAccountKeyNotSpecified,

        /// <summary>
        /// Blob endpoint {0} is not a valid https blob service endpoint for storage account {1}. Note that custom domain names are not supported.
        /// </summary>
        InvalidDiagnosticsStorageAccountBlobEndpoint,

        /// <summary>
        /// Table endpoint {0} is not a valid https table service endpoint for storage account {1}. Note that custom domain names are not supported.
        /// </summary>
        InvalidDiagnosticsStorageAccountTableEndpoint,

        /// <summary>
        /// Please specify both blob endpoint and table endpoint or none of them.
        /// </summary>
        InvalidDiagnosticsStorageAccountStorageServiceEndpointCombination,

        /// <summary>
        /// ClusterQuery failed. Reason: {0}.
        /// </summary>
        ClusterQueryFailed,

        /// <summary>
        /// The server timed out waiting for ClusterQuery response. Please ensure that the cluster is up and healthy.
        /// </summary>
        ClusterQueryRequestTimeout,

        /// <summary>
        /// The query response from the cluster is not valid.
        /// </summary>
        ClusterQueryResponseInvalid,

        /// <summary>
        /// Version is not a valid integer. {0}
        /// </summary>
        MRExtensionRequestVersionInvalid,

        /// <summary>
        /// Expected properties not found. {0}. Expected properties: {1}
        /// </summary>
        MRExtensionExpectedPropertiesNotFound,

        /// <summary>
        /// Unexpected properties found. {0}{1}. Expected properties: {2}
        /// </summary>
        MRExtensionUnexpectedPropertiesFound,

        /// <summary>
        /// Version is lower than minimum permitted value of {0}. {1}
        /// </summary>
        MRExtensionRequestVersionLowerThanMinVersion,

        /// <summary>
        /// 'Server AUTH Credential Type' parameter is required
        /// </summary>
        ServerAuthCredentialTypeRequired,

        /// <summary>
        /// The request does not fulfill requirements for
        /// a semi-privileged managed tenant type. {0}
        /// </summary>
        MRExtensionRequestDoesNotFulfillRequirementForSemiPrivileged,

        /// <summary>
        /// Feature {0} is not enabled for subscription {1}.
        /// </summary>
        FeatureNotAllowedForSubscription,

        /// <summary>
        /// Model validation error. {0}
        /// </summary>
        BestPracticesAnalyzerModelInvalid,

        /// <summary>
        /// A configuration upgrade with identical configuration version is not allowed.
        /// </summary>
        UpgradeWithIdenticalConfigVersionNotAllowed,

        /// <summary>
        /// The current state is inconsistent with the state machine: {0}.
        /// </summary>
        OperationInconsistentWithStateMachine,

        /// <summary>
        /// In KTL Logger section, if sharedLogFilePath is not null, sharedLogFileId must have value.
        /// </summary>
        SharedLogFileIdCannotBeNull,

        /// <summary>
        /// In KTL Logger section, if sharedLogFileId is not null, sharedLogFilePath must have value.
        /// </summary>
        SharedLogFilePathCannotBeNull,

        /// <summary>
        /// Primary node type node count needs to be greater than or equals to 3. Primary node type node count in the current ClusterConfig is {0}.
        /// </summary>
        PrimaryNodeCountNeedsToBeGreaterThanOrEqualsTo3,

        /// <summary>
        /// Removing Repair Manager Service is not supported.
        /// </summary>
        RemoveRepairManagerUnsupported,

        /// <summary>
        /// Certificate issuer thumbprints {0} are either invalid or contain duplicated entries.
        /// </summary>
        InvalidCertificateIssuerThumbprints,

        /// <summary>
        /// Common name must not be empty.
        /// </summary>
        InvalidCommonName,

        /// <summary>
        /// Issuer thumbprint is not supported for this particular certificate type yet.
        /// </summary>
        UnsupportedIssuerThumbprintPair,

        /// <summary>
        /// Up to 2 common names are supported for certificates except the client certificates.
        /// </summary>
        InvalidCommonNameCount,

        /// <summary>
        /// Common names and issuer thumbprints should not be both defined for a particular certificate.
        /// </summary>
        InvalidCommonNameThumbprintPair,

        /// <summary>
        /// The current and updated issuer thumbprint collections must have an intersection.
        /// </summary>
        IssuerThumbprintUpgradeWithNoIntersection,

        /// <summary>
        /// The current and updated issuer store common name must have an intersection.
        /// </summary>
        IssuerStoreCNUpgradeWithNoIntersection,

        /// <summary>
        /// The current and updated issuer store X509StoreName must have an intersection.
        /// </summary>
        IssuerStoreX509StoreNameUpgradeWithNoIntersection,

        /// <summary>
        /// Duplicated common names for the cluster certificate type is not supported.
        /// </summary>
        DupeCommonNameNotAllowedForClusterCert,

        /// <summary>
        /// Cluster certificate {0} issued by {1} is installed. Please uninstall it from all nodes before cluster config upgrade.
        /// </summary>
        CertIssuedByIssuerInstalled,

        /// <summary>
        /// Cluster name can't be changed in cluster configuration upgrade.
        /// </summary>
        ClusterNameCantBeChanged,

        /// <summary>
        /// The node details in the JSON config have been modified. In configuration upgrade, IP address and NodeTypeRef can't be changed for the same node. Violation is found in node {0}(original name). If you want to change the IP address or NodeTypeRef, please remove the old node and add a new one.
        /// </summary>
        NodeIPNodeTypeRefCantBeChanged,

        /// <summary>
        /// The node details in the JSON config have been modified. In configuration upgrade, NodeName and NodeTypeRef can't be changed for the same IP address. Violation is found in node {0}(original name). If you want to change the NodeName or NodeTypeRef, please remove the old node and add a new one.
        /// </summary>
        NodeNameNodeTypeRefCantBeChanged,

        /// <summary>
        /// Duplicate node name is detected.
        /// </summary>
        NodeNameDuplicateDetected,

        /// <summary>
        /// In UserDefined section, duplicate parameters are not allowed.
        /// </summary>
        DuplicateParameterNotAllowed,

        /// <summary>
        /// Logical directory mounted to value cannot be null.
        /// </summary>
        LogicalDirectoryMountedToValueCannotBeNull,

        /// <summary>
        /// Logical Application directory name cannot be null.
        /// </summary>
        LogicalDirectoryNameCannotBeNull,

        /// <summary>
        /// Duplicate logical directories names are not allowed.
        /// </summary>
        DuplicateLogicalDirectoriesNameDetected,

        /// <summary>
        /// Invalid client certificate thumbprint is not allowed.
        /// </summary>
        InvalidClientCertificateThumbprint,

        /// <summary>
        /// Invalid cluster certificate thumbprint is not allowed.
        /// </summary>
        InvalidClusterCertificateThumbprint,

        /// <summary>
        /// Invalid server certificate thumbprint is not allowed.
        /// </summary>
        InvalidServerCertificateThumbprint,

        /// <summary>
        /// Invalid reverse proxy certificate thumbprint is not allowed.
        /// </summary>
        InvalidReverseProxyCertificateThumbprint,

        /// <summary>
        /// IssuerStore cannot be specified without the corresponding certificate common name.
        /// </summary>
        IssuerCertStoreSpecifiedWithoutCommonNameCertificate,

        /// <summary>
        /// Up to 2 issuer common names are supported for cluster and server issuer certificates.
        /// </summary>
        IssuerCertStoreCNMoreThan2,

        /// <summary>
        /// Duplicate IssuerCommonName cannot be specified for issuer certificate stores.
        /// </summary>
        DupIssuerCertificateCN,

        /// <summary>
        /// Duplicate entries in X509StoreNames is not allowed for the same IssuerCommonName.
        /// </summary>
        DupIssuerCertificateStore,

        /// <summary>
        /// IssuerCommonName cannot be null. Specify a valid certificate common name or an empty string.
        /// </summary>
        IssuerCNCannotBeNull,

        /// <summary>
        /// Issuer X509StoreNames cannot be null. Specify a valid store name.
        /// </summary>
        IssuerStoreNameCannotBeNull,

        /// <summary>
        /// CommonNames with CertificateIssuerThumbprint and IssuerStores are not supported together. Either specify the issuer store or use explicit issuer pinning.
        /// </summary>
        IssuerCertStoreAndIssuerPinningCannotBeTogether
    }
}