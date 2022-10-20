;/*--
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
; Abstract:
;   This module contains the specific error codes and messages returned
;   by SF modules
;
; Author: Mert Coskun (MCoskun)   12-12-2016
; Environment: User and Kernel modes
;
; --*/
;
;#pragma once
;
;
;//
;// Facility codes.  Define SF to be Facility 0x43; Other components that use SF will use other facility codes.
;// Warning avoid using the commented out facility codes since they are used in KTL.
;//
;
;// const USHORT FacilityKtl = 0xFF;
;// const USHORT FacilityLogger = 0xFC;
;// const USHORT FacilityBtree = 0xBB;
;const USHORT FacilitySF = 0x43;
;
;//
;// Macro for defining NTSTATUS codes.
;//
;
;#define SfStatusCode(Severity, Facility, ErrorCode) ((NTSTATUS) ((Severity)<<30) | (1<<29) | ((Facility)<<16) | ErrorCode)
;
;//
;// Macro for extracting facility code from a SF NTSTATUS code.
;//
;
;#define SfStatusFacilityCode(ErrorCode) (((ErrorCode) >> 16) & 0xFFFF)
;

MessageIdTypedef = NTSTATUS

SeverityNames =
(
    Success         = 0x0 : STATUS_SEVERITY_SUCCESS
    Informational   = 0x1 : STATUS_SEVERITY_INFORMATIONAL
    Warning         = 0x2 : STATUS_SEVERITY_WARNING
    Error           = 0x3 : STATUS_SEVERITY_ERROR
)

FacilityNames =
(
    FacilitySF      = 0x43 : FACILITY_SF
)


;/***************************************************************************/
;/*                   NTSTATUS codes for SF                                 */
;/*                                                                         */
;/* Note: All codes must match FABRIC_ERROR_CODEs : [1bbc, 1d4b]            */
;/* Note: Due to conversion requirements between HResult,                   */
;/*       only Error and Success Severity can be used                       */
;/***************************************************************************/
MessageId    = 0x1bbc
SymbolicName = SF_STATUS_COMMUNICATION_ERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
A communication error caused the operation to fail.
.

MessageId    = 0x1bbd
SymbolicName = SF_STATUS_INVALID_ADDRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
The supplied address was invalid.
.

MessageId    = 0x1bbe
SymbolicName = SF_STATUS_INVALID_NAME_URI
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid name URI.
.

MessageId    = 0x1bbf
SymbolicName = SF_STATUS_INVALID_PARTITION_KEY
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid partition key or ID.
.

MessageId    = 0x1bc0
SymbolicName = SF_STATUS_NAME_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Name already exists.
.

MessageId    = 0x1bc1
SymbolicName = SF_STATUS_NAME_DOES_NOT_EXIST
Facility     = FacilitySF
Severity     = Error
Language     = English
Name does not exist.
.

MessageId    = 0x1bc2
SymbolicName = SF_STATUS_NAME_NOT_EMPTY
Facility     = FacilitySF
Severity     = Error
Language     = English
Name not empty.
.

MessageId    = 0x1bc3
SymbolicName = SF_STATUS_NODE_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Node not found.
.

MessageId    = 0x1bc4
SymbolicName = SF_STATUS_NODE_IS_UP
Facility     = FacilitySF
Severity     = Error
Language     = English
Node is up.
.

MessageId    = 0x1bc5
SymbolicName = SF_STATUS_NO_WRITE_QUORUM
Facility     = FacilitySF
Severity     = Error
Language     = English
he operation failed because a quorum of nodes are not available for this replica set.  Consider retrying once more nodes are up.
.

MessageId    = 0x1bc6
SymbolicName = SF_STATUS_NOT_PRIMARY
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because this node is not the primary replica.  Consider re-resolving the primary and retrying the operation there.
.

MessageId    = 0x1bc7
SymbolicName = SF_STATUS_NOT_READY
Facility     = FacilitySF
Severity     = Error
Language     = English
The requested data is not yet in cache.  This same operation is likely to succeed if retried later.
.

MessageId    = 0x1bc8
SymbolicName = SF_STATUS_OPERATION_NOT_COMPLETE
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation has not completed.
.

MessageId    = 0x1bc9
SymbolicName = SF_STATUS_PROPERTY_DOES_NOT_EXIST
Facility     = FacilitySF
Severity     = Error
Language     = English
Property does not exist.
.

MessageId    = 0x1bca
SymbolicName = SF_STATUS_RECONFIGURATION_PENDING
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because this replica set is currently reconfiguring.
.

MessageId    = 0x1bcb
SymbolicName = SF_STATUS_REPLICATION_QUEUE_FULL
Facility     = FacilitySF
Severity     = Error
Language     = English
Replication queue is full.
.

MessageId    = 0x1bcc
SymbolicName = SF_SERVICE_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Service already exists.
.

MessageId    = 0x1bcd
SymbolicName = SF_SERVICE_DOES_NOT_EXIST
Facility     = FacilitySF
Severity     = Error
Language     = English
Service does not exist.
.

MessageId    = 0x1bce
SymbolicName = SF_STATUS_SERVICE_OFFLINE
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because the service is currently offline.
.

MessageId    = 0x1bcf
SymbolicName = SF_STATUS_SERVICE_METADATA_MISMATCH  
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because the service metadata does not match.
.

MessageId    = 0x1bd0
SymbolicName = SF_STATUS_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED  
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because creating the service would form an affinity chain which is not supported.
.

MessageId    = 0x1bd1
SymbolicName = SF_STATUS_SERVICE_TYPE_ALREADY_REGISTERED 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Type is already registered.
.

MessageId    = 0x1bd2
SymbolicName = SF_STATUS_SERVICE_TYPE_NOT_REGISTERED 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Type is not registered.
.

MessageId    = 0x1bd3
SymbolicName = SF_STATUS_VALUE_TOO_LARGE 
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid argument
.

MessageId    = 0x1bd4
SymbolicName = SF_STATUS_VALUE_EMPTY 
Facility     = FacilitySF
Severity     = Error
Language     = English
The property value is empty.
.

MessageId    = 0x1bd5
SymbolicName = SF_STATUS_PROPERTY_CHECK_FAILED 
Facility     = FacilitySF
Severity     = Error
Language     = English
The property check failed.
.

MessageId    = 0x1bd6
SymbolicName = SF_STATUS_WRITE_CONFLICT 
Facility     = FacilitySF
Severity     = Error
Language     = English
Write conflict.
.

MessageId    = 0x1bd7
SymbolicName = SF_STATUS_ENUMERATION_COMPLETED 
Facility     = FacilitySF
Severity     = Error
Language     = English
Enumeration has completed.
.

MessageId    = 0x1bd8
SymbolicName = SF_STATUS_APPLICATION_TYPE_PROVISION_IN_PROGRESS 
Facility     = FacilitySF
Severity     = Error
Language     = English
Cannot provision multiple versions of the same application type simultaneously.
.

MessageId    = 0x1bd9
SymbolicName = SF_STATUS_APPLICATION_TYPE_ALREADY_EXISTS 
Facility     = FacilitySF
Severity     = Error
Language     = English
Application type and version already exists
.

MessageId    = 0x1bda
SymbolicName = SF_STATUS_APPLICATION_TYPE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
Application type and version not found
.

MessageId    = 0x1bdb
SymbolicName = SF_STATUS_APPLICATION_TYPE_IN_USE 
Facility     = FacilitySF
Severity     = Error
Language     = English
Application type and version is still in use
.

MessageId    = 0x1bdc
SymbolicName = SF_STATUS_APPLICATION_ALREADY_EXISTS 
Facility     = FacilitySF
Severity     = Error
Language     = English
APPLICATION_ALREADY_EXISTS 
.

MessageId    = 0x1bdd
SymbolicName = SF_STATUS_APPLICATION_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
Application not found
.

MessageId    = 0x1bde
SymbolicName = SF_STATUS_APPLICATION_UPGRADE_IN_PROGRESS 
Facility     = FacilitySF
Severity     = Error
Language     = English
Application is currently being upgraded
.

MessageId    = 0x1bdf
SymbolicName = SF_STATUS_APPLICATION_UPGRADE_VALIDATION_ERROR 
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid application upgrade request
.

MessageId    = 0x1be0
SymbolicName = SF_STATUS_SERVICE_TYPE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Type not found
.

MessageId    = 0x1be1
SymbolicName = SF_STATUS_SERVICE_TYPE_MISMATCH 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Type mismatches description type (i.e. stateless/stateful/persisted)
.

MessageId    = 0x1be2
SymbolicName = SF_STATUS_SERVICE_TYPE_TEMPLATE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Type template not found
.

MessageId    = 0x1be3
SymbolicName = SF_STATUS_CONFIGURATION_SECTION_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
A configuration section is missing.
.

MessageId    = 0x1be4
SymbolicName = SF_STATUS_CONFIGURATION_PARAMETER_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
A configuration parameter is missing.
.

MessageId    = 0x1be5
SymbolicName = SF_STATUS_INVALID_CONFIGURATION 
Facility     = FacilitySF
Severity     = Error
Language     = English
A configuration parameter has been set to an invalid value. Please check the logs for details.
.

MessageId    = 0x1be6
SymbolicName = SF_STATUS_IMAGEBUILDER_VALIDATION_ERROR 
Facility     = FacilitySF
Severity     = Error
Language     = English
There is a content validation error in the manifest file(s)
.

MessageId    = 0x1be7
SymbolicName = SF_STATUS_PARTITION_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation failed because the requested partition does not exist.
.

MessageId    = 0x1be8
SymbolicName = SF_STATUS_REPLICA_DOES_NOT_EXIST 
Facility     = FacilitySF
Severity     = Error
Language     = English
The specified replica does not exist.
.

MessageId    = 0x1be9
SymbolicName = SF_STATUS_SERVICE_GROUP_ALREADY_EXISTS 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service group already exists.
.

MessageId    = 0x1bea
SymbolicName = SF_STATUS_SERVICE_GROUP_DOES_NOT_EXIST 
Facility     = FacilitySF
Severity     = Error
Language     = English
Service group does not exist.
.

MessageId    = 0x1beb
SymbolicName = SF_STATUS_PROCESS_DEACTIVATED 
Facility     = FacilitySF
Severity     = Error
Language     = English
The process has been deactivated.
.

MessageId    = 0x1bec
SymbolicName = SF_STATUS_PROCESS_ABORTED 
Facility     = FacilitySF
Severity     = Error
Language     = English
The process has aborted.
.

MessageId    = 0x1ed
SymbolicName = SF_STATUS_UPGRADE_FAILED 
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric upgrade failed on the node.
.

MessageId    = 0x1bee
SymbolicName = SF_STATUS_INVALID_CREDENTIAL_TYPE 
Facility     = FacilitySF
Severity     = Error
Language     = English
The security credential type specified is invalid.
.


MessageId    = 0x1bef
SymbolicName = SF_STATUS_INVALID_X509_FIND_TYPE 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 find type specified is invalid.
.


MessageId    = 0x1bf0
SymbolicName = SF_STATUS_INVALID_X509_STORE_LOCATION 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 store location specified is invalid.
.

MessageId    = 0x1bf1
SymbolicName = SF_STATUS_INVALID_X509_STORE_NAME 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 store name specified is invalid.
.

MessageId    = 0x1bf2
SymbolicName = SF_STATUS_INVALID_X509_THUMBPRINT 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 thumbprint specified is invalid.
.

MessageId    = 0x1bf3
SymbolicName = SF_STATUS_INVALID_PROTECTION_LEVEL 
Facility     = FacilitySF
Severity     = Error
Language     = English
The protection level specified is invalid.
.

MessageId    = 0x1bf4
SymbolicName = SF_STATUS_INVALID_X509_STORE 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 store specified is invalid.
.

MessageId    = 0x1bf5
SymbolicName = SF_STATUS_INVALID_SUBJECT_NAME 
Facility     = FacilitySF
Severity     = Error
Language     = English
The X509 subject name specified is invalid.
.

MessageId    = 0x1bf6
SymbolicName = SF_STATUS_INVALID_ALLOWED_COMMON_NAME_LIST 
Facility     = FacilitySF
Severity     = Error
Language     = English
The specified list of allowed common names is invalid.
.

MessageId    = 0x1bf7
SymbolicName = SF_STATUS_INVALID_CREDENTIALS 
Facility     = FacilitySF
Severity     = Error
Language     = English
The specified credentials are invalid.
.

MessageId    = 0x1bf8
SymbolicName = SF_STATUS_DECRYPTION_FAILED 
Facility     = FacilitySF
Severity     = Error
Language     = English
Failed to decrypt the fvalue.
.

MessageId    = 0x1bf9
SymbolicName = SF_STATUS_CONFIGURATION_PACKAGE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
The config package was not found
.

MessageId    = 0x1bfa
SymbolicName = SF_STATUS_DATA_PACKAGE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
The data package was not found
.

MessageId    = 0x1bfb
SymbolicName = SF_STATUS_CODE_PACKAGE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
The code package was not found
.

MessageId    = 0x1bfc
SymbolicName = SF_STATUS_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND 
Facility     = FacilitySF
Severity     = Error
Language     = English
The Endpoint resource was not found
.

MessageId    = 0x1bfd
SymbolicName = SF_STATUS_INVALID_OPERATION
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation is invalid.
.

MessageId    = 0x1bfe
SymbolicName = SF_STATUS_OBJECT_CLOSED
Facility     = FacilitySF
Severity     = Error
Language     = English
The object is closed.
.

MessageId    = 0x1bff
SymbolicName = SF_STATUS_TIMEOUT
Facility     = FacilitySF
Severity     = Error
Language     = English
Operation timed out.
.

MessageId    = 0x1c00
SymbolicName = SF_STATUS_FILE_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
File not found.
.

MessageId    = 0x1c01
SymbolicName = SF_STATUS_DIRECTORY_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Directory not found.
.

MessageId    = 0x1c02
SymbolicName = SF_STATUS_INVALID_DIRECTORY
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid Directory.
.

MessageId    = 0x1c03
SymbolicName = SF_STATUS_PATH_TOO_LONG
Facility     = FacilitySF
Severity     = Error
Language     = English
Path is too long.
.

MessageId    = 0x1c04
SymbolicName = SF_STATUS_IMAGESTORE_IOERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
There was an IOException when using the Image Store.
.

MessageId    = 0x1c05
SymbolicName = SF_STATUS_CORRUPTED_IMAGE_STORE_OBJECT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
The Image Store object related to this operation appears corrupted. Please re-upload to the Image Store incoming folder before retrying the operation.
.

MessageId    = 0x1c06
SymbolicName = SF_STATUS_APPLICATION_NOT_UPGRADING
Facility     = FacilitySF
Severity     = Error
Language     = English
Application is currently NOT being upgraded
.

MessageId    = 0x1c07
SymbolicName = SF_STATUS_APPLICATION_ALREADY_IN_TARGET_VERSION
Facility     = FacilitySF
Severity     = Error
Language     = English
Application is already in the target version
.

MessageId    = 0x1c08
SymbolicName = SF_STATUS_IMAGEBUILDER_UNEXPECTED_ERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
The Image Builder encountered an unexpected error.
.

MessageId    = 0x1c09
SymbolicName = SF_STATUS_FABRIC_VERSION_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric version has not been registered
.

MessageId    = 0x1c0a
SymbolicName = SF_STATUS_FABRIC_VERSION_IN_USE
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric is currently in this version or being upgraded to this version
.

MessageId    = 0x1c0b
SymbolicName = SF_STATUS_FABRIC_VERSION_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Version has already been registered
.

MessageId    = 0x1c0c
SymbolicName = SF_STATUS_FABRIC_ALREADY_IN_TARGET_VERSION
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric is already in this version
.

MessageId    = 0x1c0d
SymbolicName = SF_STATUS_FABRIC_NOT_UPGRADING
Facility     = FacilitySF
Severity     = Error
Language     = English
There is no pending Fabric upgrade
.

MessageId    = 0x1c0e
SymbolicName = SF_STATUS_FABRIC_UPGRADE_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric is being upgraded
.

MessageId    = 0x1c0f
SymbolicName = SF_STATUS_FABRIC_UPGRADE_VALIDATION_ERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric upgrade request failed validation
.

MessageId    = 0x1c10
SymbolicName = SF_STATUS_HEALTH_MAX_REPORTS_REACHED
Facility     = FacilitySF
Severity     = Error
Language     = English
Max number of health reports reached, try again
.

MessageId    = 0x1c11
SymbolicName = SF_STATUS_HEALTH_STALE_REPORT
Facility     = FacilitySF
Severity     = Error
Language     = English
Health report is stale
.

MessageId    = 0x1c12
SymbolicName = SF_STATUS_KEY_TOO_LARGE
Facility     = FacilitySF
Severity     = Error
Language     = English
The key is too large.
.

MessageId    = 0x1c13
SymbolicName = SF_STATUS_KEY_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
The given key was not present.
.

MessageId    = 0x1c14
SymbolicName = SF_STATUS_SEQUENCE_NUMBER_CHECK_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Sequence number check failed
.

MessageId    = 0x1c15
SymbolicName = SF_STATUS_ENCRYPTION_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Failed to encrypt the value.
.

MessageId    = 0x1c16
SymbolicName = SF_STATUS_INVALID_ATOMIC_GROUP
Facility     = FacilitySF
Severity     = Error
Language     = English
The specified atomic group has not been created or no longer exists.
.

MessageId    = 0x1c17
SymbolicName = SF_STATUS_HEALTH_ENTITY_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Entity not found in Health Store
.

MessageId    = 0x1c18
SymbolicName = SF_STATUS_SERVICE_MANIFEST_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Manifest not found
.

MessageId    = 0x1c19
SymbolicName = SF_STATUS_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session transport startup failure
.

MessageId    = 0x1c1a
SymbolicName = SF_STATUS_RELIABLE_SESSION_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session already exists
.

MessageId    = 0x1c1b
SymbolicName = SF_STATUS_RELIABLE_SESSION_CANNOT_CONNECT
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session cannot connect
.

MessageId    = 0x1c1c
SymbolicName = SF_STATUS_RELIABLE_SESSION_MANAGER_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session manager exists
.

MessageId    = 0x1c1d
SymbolicName = SF_STATUS_RELIABLE_SESSION_REJECTED
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session rejected
.

MessageId    = 0x1c1e
SymbolicName = SF_STATUS_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session manager already listening
.

MessageId    = 0x1c1f
SymbolicName = SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session manager not found
.

MessageId    = 0x1c20
SymbolicName = SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_LISTENING
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session manager not listening
.

MessageId    = 0x1c21
SymbolicName = SF_STATUS_INVALID_SERVICE_TYPE
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid Service Type
.

MessageId    = 0x1c22
SymbolicName = SF_STATUS_IMAGEBUILDER_TIMEOUT
Facility     = FacilitySF
Severity     = Error
Language     = English
Image Builder timed out. Please check that the Image Store is available and consider providing a larger timeout when processing large application packages.
.

MessageId    = 0x1c23
SymbolicName = SF_STATUS_IMAGEBUILDER_ACCESS_DENIED
Facility     = FacilitySF
Severity     = Error
Language     = English
Please check that the Image Store is available and has correct access permissions for Microsoft Azure Service Fabric processes.
.

MessageId    = 0x1c24
SymbolicName = SF_STATUS_IMAGEBUILDER_INVALID_MSI_FILE
Facility     = FacilitySF
Severity     = Error
Language     = English
The MSI file is invalid.
.

MessageId    = 0x1c25
SymbolicName = SF_STATUS_SERVICE_TOO_BUSY
Facility     = FacilitySF
Severity     = Error
Language     = English
The service cannot process the request because it is too busy. Please retry.
.

MessageId    = 0x1c26
SymbolicName = SF_STATUS_TRANSACTION_NOT_ACTIVE
Facility     = FacilitySF
Severity     = Error
Language     = English
Transaction has already committed or rolled back
.

MessageId    = 0x1c27
SymbolicName = SF_STATUS_REPAIR_TASK_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
The repair task already exists.
.

MessageId    = 0x1c28
SymbolicName = SF_STATUS_REPAIR_TASK_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
The repair task could not be found.
.

MessageId    = 0x1c29
SymbolicName = SF_STATUS_RELIABLE_SESSION_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session not found
.

MessageId    = 0x1c2a
SymbolicName = SF_STATUS_RELIABLE_SESSION_QUEUE_EMPTY
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session queue empty
.

MessageId    = 0x1c2b
SymbolicName = SF_STATUS_RELIABLE_SESSION_QUOTA_EXCEEDED
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session quota exceeded
.

MessageId    = 0x1c2c
SymbolicName = SF_STATUS_RELIABLE_SESSION_SERVICE_FAULTED
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session service faulted
.

MessageId    = 0x1c2d
SymbolicName = SF_STATUS_RELIABLE_SESSION_INVALID_TARGET_PARTITION
Facility     = FacilitySF
Severity     = Error
Language     = English
Reliable session invalid target partition provided
.

MessageId    = 0x1c2e
SymbolicName = SF_STATUS_TRANSACTION_TOO_LARGE
Facility     = FacilitySF
Severity     = Error
Language     = English
Transaction data exceeds the configured replication message size limit - ReplicatorSettings.MaxReplicationMessageSize
.

MessageId    = 0x1c2f
SymbolicName = SF_STATUS_REPLICATION_OPERATION_TOO_LARGE
Facility     = FacilitySF
Severity     = Error
Language     = English
The replication operation is larger than the configured limit - MaxReplicationMessageSize
.

MessageId    = 0x1c30
SymbolicName = SF_STATUS_INSTANCE_ID_MISMATCH
Facility     = FacilitySF
Severity     = Error
Language     = English
The provided InstanceId did not match.
.

MessageId    = 0x1c31
SymbolicName = SF_STATUS_UPGRADE_DOMAIN_ALREADY_COMPLETED
Facility     = FacilitySF
Severity     = Error
Language     = English
Specified upgrade domain has already completed.
.

MessageId    = 0x1c32
SymbolicName = SF_STATUS_NODE_HAS_NOT_STOPPED_YET
Facility     = FacilitySF
Severity     = Error
Language     = English
Node has not stopped yet - a previous StopNode is still pending
.

MessageId    = 0x1c33
SymbolicName = SF_STATUS_INSUFFICIENT_CLUSTER_CAPACITY
Facility     = FacilitySF
Severity     = Error
Language     = English
The cluster did not have enough resources to perform the requested operation.
.

MessageId    = 0x1c34
SymbolicName = SF_STATUS_INVALID_PACKAGE_SHARING_POLICY
Facility     = FacilitySF
Severity     = Error
Language     = English
PackageSharingPolicy must contain a valid PackageName or Scope
.

MessageId    = 0x1c35
SymbolicName = SF_STATUS_PREDEPLOYMENT_NOT_ALLOWED
Facility     = FacilitySF
Severity     = Error
Language     = English
Service Manifest packages could not be deployed to node because Image Cache is disabled
.

MessageId    = 0x1c36
SymbolicName = SF_STATUS_INVALID_BACKUP_SETTING
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid backup setting. E.g. incremental backup option is not set upfront
.

MessageId    = 0x1c37
SymbolicName = SF_STATUS_MISSING_FULL_BACKUP
Facility     = FacilitySF
Severity     = Error
Language     = English
Incremental backups can only be done after an initial full backup
.

MessageId    = 0x1c38
SymbolicName = SF_STATUS_BACKUP_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
A previously initiated backup is currently in progress
.

MessageId    = 0x1c39
SymbolicName = SF_STATUS_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME
Facility     = FacilitySF
Severity     = Error
Language     = English
Service notification filter has already been registered at the specified name.
.

MessageId    = 0x1c3a
SymbolicName = SF_STATUS_INVALID_REPLICA_OPERATION
Facility     = FacilitySF
Severity     = Error
Language     = English
Restart can only be performed on persisted services. For volatile or stateless services use Remove.
.

MessageId    = 0x1c3b
SymbolicName = SF_STATUS_INVALID_REPLICA_STATE
Facility     = FacilitySF
Severity     = Error
Language     = English
This operation cannot be performed in the current replica state.
.

MessageId    = 0x1c3c
SymbolicName = SF_STATUS_LOADBALANCER_NOT_READY
Facility     = FacilitySF
Severity     = Error
Language     = English
Load Balancer is currently busy.
.

MessageId    = 0x1c3d
SymbolicName = SF_STATUS_INVALID_PARTITION_OPERATION
Facility     = FacilitySF
Severity     = Error
Language     = English
MovePrimary or MoveSecondary operation can be only be performed on stateful Service Type.
.

MessageId    = 0x1c3e
SymbolicName = SF_STATUS_PRIMARY_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Replica is already primary role.
.

MessageId    = 0x1c3f
SymbolicName = SF_STATUS_SECONDARY_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Replica is already secondary role.
.

MessageId    = 0x1c40
SymbolicName = SF_STATUS_BACKUP_DIRECTORY_NOT_EMPTY
Facility     = FacilitySF
Severity     = Error
Language     = English
The backup directory is not empty
.

MessageId    = 0x1c41
SymbolicName = SF_STATUS_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION
Facility     = FacilitySF
Severity     = Error
Language     = English
Replicas hosted in Fabric.exe or in processes not managed by Microsoft Azure Service Fabric cannot be force removed.
.

MessageId    = 0x1c42
SymbolicName = SF_STATUS_ACQUIRE_FILE_LOCK_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
There was an error acquiring lock on the file. This indicates that another process has acquired write lock on the file or the process does not have access to the file location.
.

MessageId    = 0x1c43
SymbolicName = SF_STATUS_CONNECTION_DENIED
Facility     = FacilitySF
Severity     = Error
Language     = English
Not authorized to connect
.

MessageId    = 0x1c44
SymbolicName = SF_STATUS_SERVER_AUTHENTICATION_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Failed to authenticate server identity
.

MessageId    = 0x1c45
SymbolicName = SF_STATUS_CONSTRAINT_KEY_UNDEFINED
Facility     = FacilitySF
Severity     = Error
Language     = English
One or more placement constraints on the service are undefined on all nodes that are currently up.
.

MessageId    = 0x1c46
SymbolicName = SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED
Facility     = FacilitySF
Severity     = Error
Language     = English
Multithreaded usage of transactions is not allowed.
.

MessageId    = 0x1c47
SymbolicName = SF_STATUS_INVALID_X509_NAME_LIST
Facility     = FacilitySF
Severity     = Error
Language     = English

.

MessageId    = 0x1c48
SymbolicName = SF_STATUS_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED
Facility     = FacilitySF
Severity     = Error
Language     = English

.

MessageId    = 0x1c49
SymbolicName = SF_STATUS_GATEWAY_NOT_REACHABLE
Facility     = FacilitySF
Severity     = Error
Language     = English
A communication error caused the operation to fail.
.

MessageId    = 0x1c4a
SymbolicName = SF_STATUS_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED
Facility     = FacilitySF
Severity     = Error
Language     = English
Certificate for UserRole FabricClient is not configured.
.

MessageId    = 0x1c4b
SymbolicName = SF_STATUS_TRANSACTION_ABORTED
Facility     = FacilitySF
Severity     = Error
Language     = English
Transaction cannot be used after encountering an error. Retries must occur on a new transaction.
.

MessageId    = 0x1c4c
SymbolicName = SF_STATUS_CANNOT_CONNECT
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates there was a connection failure.
.

MessageId    = 0x1c4d
SymbolicName = SF_STATUS_MESSAGE_TOO_LARGE
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the message is too large.
.

MessageId    = 0x1c4e
SymbolicName = SF_STATUS_CONSTRAINT_NOT_SATISFIED
Facility     = FacilitySF
Severity     = Error
Language     = English
The service's and cluster's configuration settings would result in a constraint-violating state if the operation were executed.
.

MessageId    = 0x1c4f
SymbolicName = SF_STATUS_ENDPOINT_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the specified endpoint was not found.
.

MessageId    = 0x1c50
SymbolicName = SF_STATUS_APPLICATION_UPDATE_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the application is currently being updated.
.

MessageId    = 0x1c51
SymbolicName = SF_STATUS_DELETE_BACKUP_FILE_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Deletion of backup files/directory failed. Currently this can happen in a scenario where backup is used mainly to truncate logs.
.

MessageId    = 0x1c52
SymbolicName = SF_STATUS_CONNECTION_CLOSED_BY_REMOTE_END
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the connection was closed by the remote end.
.

MessageId    = 0x1c53
SymbolicName = SF_STATUS_INVALID_TEST_COMMAND_STATE
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates that this API call is not valid for the current state of the test command.
.

MessageId    = 0x1c54
SymbolicName = SF_STATUS_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates that this test command operation id (Guid) is already being used.
.

MessageId    = 0x1c55
SymbolicName = SF_STATUS_CM_OPERATION_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Creation or deletion terminated due to persistent failures after bounded retry.
.

MessageId    = 0x1c56
SymbolicName = SF_STATUS_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the path passed by the user starts with a reserved directory.
.

MessageId    = 0x1c57
SymbolicName = SF_STATUS_CERTIFICATE_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates the certificate is not found.
.

MessageId    = 0x1c58
SymbolicName = SF_STATUS_CHAOS_ALREADY_RUNNING
Facility     = FacilitySF
Severity     = Error
Language     = English
A FabricErrorCode that indicates that an instance of Chaos is already running.
.

MessageId    = 0x1c59
SymbolicName = SF_STATUS_FABRIC_DATA_ROOT_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Fabric Data Root is not defined on the target machine.
.

MessageId    = 0x1c5a
SymbolicName = SF_STATUS_INVALID_RESTORE_DATA
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that restore metadata present in supplied restore directory in invalid.
.

MessageId    = 0x1c5b
SymbolicName = SF_STATUS_DUPLICATE_BACKUPS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that backup-chain in specified restore directory contains duplicate backups.
.

MessageId    = 0x1c5c
SymbolicName = SF_STATUS_INVALID_BACKUP_CHAIN
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that backup-chain in specified restore directory has one or more missing backups.
.

MessageId    = 0x1c5d
SymbolicName = SF_STATUS_STOP_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
An operation is already in progress.
.

MessageId    = 0x1c5e
SymbolicName = SF_STATUS_ALREADY_STOPPED
Facility     = FacilitySF
Severity     = Error
Language     = English
The node is already in a stopped state.
.

MessageId    = 0x1c5f
SymbolicName = SF_STATUS_NODE_IS_DOWN
Facility     = FacilitySF
Severity     = Error
Language     = English
The node is down (not stopped).
.

MessageId    = 0x1c60
SymbolicName = SF_STATUS_NODE_TRANSITION_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
Node transition in progress.
.

MessageId    = 0x1c61
SymbolicName = SF_STATUS_INVALID_BACKUP
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that backup provided for restore is invalid.
.

MessageId    = 0x1c62
SymbolicName = SF_STATUS_INVALID_INSTANCE_ID
Facility     = FacilitySF
Severity     = Error
Language     = English
The provided instance id is invalid.
.

MessageId    = 0x1c63
SymbolicName = SF_STATUS_INVALID_DURATION
Facility     = FacilitySF
Severity     = Error
Language     = English
The provided duration is invalid.
.

MessageId    = 0x1c64
SymbolicName = SF_STATUS_RESTORE_SAFE_CHECK_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that backup provided for restore has older data than present in service.
.

MessageId    = 0x1c65
SymbolicName = SF_STATUS_CONFIG_UPGRADE_FAILED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the config upgrade fails.
.

MessageId    = 0x1c66
SymbolicName = SF_STATUS_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the upload session range will overlap or are out of range.
.

MessageId    = 0x1c67
SymbolicName = SF_STATUS_UPLOAD_SESSION_ID_CONFLICT
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the upload session ID is existed for a different image store relative path.
.

MessageId    = 0x1c68
SymbolicName = SF_STATUS_INVALID_PARTITION_SELECTOR
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the partition selector is invalid.
.

MessageId    = 0x1c69
SymbolicName = SF_STATUS_INVALID_REPLICA_SELECTOR
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the replica selector is invalid.
.

MessageId    = 0x1c6a
SymbolicName = SF_STATUS_DNS_SERVICE_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that DnsService is not enabled on the cluster.
.

MessageId    = 0x1c6b
SymbolicName = SF_STATUS_INVALID_DNS_NAME
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that service DNS name is invalid.
.

MessageId    = 0x1c6c
SymbolicName = SF_STATUS_DNS_NAME_IN_USE
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that service DNS name is in use by another service.
.

MessageId    = 0x1c6d
SymbolicName = SF_STATUS_COMPOSE_DEPLOYMENT_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the compose application already exists.
.

MessageId    = 0x1c6e
SymbolicName = SF_STATUS_COMPOSE_DEPLOYMENT_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the compose application is not found.
.

MessageId    = 0x1c6f
SymbolicName = SF_STATUS_INVALID_FOR_STATEFUL_SERVICES
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the operation is only valid for stateless services.
.

MessageId    = 0x1c70
SymbolicName = SF_STATUS_INVALID_FOR_STATELESS_SERVICES
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the operation is only valid for stateful services.
.

MessageId    = 0x1c71
SymbolicName = SF_STATUS_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the operation is only valid for stateful persistent services.
.

MessageId    = 0x1c72
SymbolicName = SF_STATUS_INVALID_UPLOAD_SESSION_ID
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the upload session ID is invalid. Please use GUID as upload session ID.
.

MessageId    = 0x1c73
SymbolicName = SF_STATUS_BACKUP_NOT_ENABLED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the backup protection is not enabled.
.

MessageId    = 0x1c74
SymbolicName = SF_STATUS_BACKUP_IS_ENABLED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the backup protection is enabled.
.

MessageId    = 0x1c75
SymbolicName = SF_STATUS_BACKUP_POLICY_DOES_NOT_EXIST
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the Backup Policy does not exist.
.

MessageId    = 0x1c76
SymbolicName = SF_STATUS_BACKUP_POLICY_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the Backup Policy is already exists.
.

MessageId    = 0x1c77
SymbolicName = SF_STATUS_RESTORE_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that a partition is already has a restore in progress.
.

MessageId    = 0x1c78
SymbolicName = SF_STATUS_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the source from where restore is requested has a properties mismatch with target partition.
.

MessageId    = 0x1c79
SymbolicName = SF_STATUS_FAULT_ANALYSIS_SERVICE_NOT_ENABLED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the Restore cannot be triggered as Fault Analysis Service is not running.
.

MessageId    = 0x1c7a
SymbolicName = SF_STATUS_CONTAINER_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the container is not found.
.

MessageId    = 0x1c7b
SymbolicName = SF_STATUS_OBJECT_DISPOSED
Facility     = FacilitySF
Severity     = Error
Language     = English
The operation is performed on a disposed object.
.

MessageId    = 0x1c7c
SymbolicName = SF_STATUS_NOT_READABLE
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the partition is not readable.
.

MessageId    = 0x1c7d
SymbolicName = SF_STATUS_BACKUPCOPIER_UNEXPECTED_ERROR
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the backup copier failed due to unexpected error during its operation.
.

MessageId    = 0x1c7e
SymbolicName = SF_STATUS_BACKUPCOPIER_TIMEOUT
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the backup copier failed due to timeout during its operation.
.

MessageId    = 0x1c7f
SymbolicName = SF_STATUS_BACKUPCOPIER_ACCESS_DENIED
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the backup copier was denied required access to complete operation.
.

MessageId    = 0x1c80
SymbolicName = SF_STATUS_INVALID_SERVICE_SCALING_POLICY
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the scaling policy specified for the service is invalid
.

MessageId    = 0x1c81
SymbolicName = SF_STATUS_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the single instance application already exists.
.

MessageId    = 0x1c82
SymbolicName = SF_STATUS_SINGLE_INSTANCE_APPLICATION_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the single instance application is not found.
.

MessageId    = 0x1c83
SymbolicName = SF_STATUS_VOLUME_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
Volume already exists.
.

MessageId    = 0x1c84
SymbolicName = SF_STATUS_VOLUME_NOT_FOUND
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the specified volume is not found.
.

MessageId    = 0x1c85
SymbolicName = SF_STATUS_DATABASE_MIGRATION_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates that the service is undergoing database migration and unavailable for writes
.

MessageId    = 0x1c86
SymbolicName = SF_STATUS_CENTRAL_SECRET_SERVICE_GENERIC
Facility     = FacilitySF
Severity     = Error
Language     = English
Central Secret Service generic error.
.

MessageId    = 0x1c87
SymbolicName = SF_STATUS_SECRET_INVALID
Facility     = FacilitySF
Severity     = Error
Language     = English
Invalid secret error.
.

MessageId    = 0x1c88
SymbolicName = SF_STATUS_SECRET_VERSION_ALREADY_EXISTS
Facility     = FacilitySF
Severity     = Error
Language     = English
The secret version already exists.
.

MessageId    = 0x1c89
SymbolicName = SF_STATUS_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS
Facility     = FacilitySF
Severity     = Error
Language     = English
Single instance application is currently being upgraded
.

MessageId    = 0x1c8a
SymbolicName = SF_STATUS_COMPOSE_DEPLOYMENT_NOT_UPGRADING
Facility     = FacilitySF
Severity     = Error
Language     = English
Indicates the compose application is not upgrading.
.

MessageId    = 0x1c8b
SymbolicName = SF_STATUS_SECRET_TYPE_CANNOT_BE_CHANGED
Facility     = FacilitySF
Severity     = Error
Language     = English
The type of an existing secret cannot be changed.
.
