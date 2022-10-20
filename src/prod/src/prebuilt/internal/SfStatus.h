/*--

 Copyright (c) Microsoft Corporation.  All rights reserved.

 Abstract:
   This module contains the specific error codes and messages returned
   by SF modules

 Author: Mert Coskun (MCoskun)   12-12-2016
 Environment: User and Kernel modes

 --*/

#pragma once


//
// Facility codes.  Define SF to be Facility 0x43; Other components that use SF will use other facility codes.
// Warning avoid using the commented out facility codes since they are used in KTL.
//

// const USHORT FacilityKtl = 0xFF;
// const USHORT FacilityLogger = 0xFC;
// const USHORT FacilityBtree = 0xBB;
const USHORT FacilitySF = 0x43;

//
// Macro for defining NTSTATUS codes.
//

#define SfStatusCode(Severity, Facility, ErrorCode) ((NTSTATUS) ((Severity)<<30) | (1<<29) | ((Facility)<<16) | ErrorCode)

//
// Macro for extracting facility code from a SF NTSTATUS code.
//

#define SfStatusFacilityCode(ErrorCode) (((ErrorCode) >> 16) & 0xFFFF)

/***************************************************************************/
/*                   NTSTATUS codes for SF                                 */
/*                                                                         */
/* Note: All codes must match FABRIC_ERROR_CODEs : [1bbc, 1d4b]            */
/* Note: Due to conversion requirements between HResult,                   */
/*       only Error and Success Severity can be used                       */
/***************************************************************************/
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SF                      0x43


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: SF_STATUS_COMMUNICATION_ERROR
//
// MessageText:
//
// A communication error caused the operation to fail.
//
#define SF_STATUS_COMMUNICATION_ERROR    ((NTSTATUS)0xE0431BBCL)

//
// MessageId: SF_STATUS_INVALID_ADDRESS
//
// MessageText:
//
// The supplied address was invalid.
//
#define SF_STATUS_INVALID_ADDRESS        ((NTSTATUS)0xE0431BBDL)

//
// MessageId: SF_STATUS_INVALID_NAME_URI
//
// MessageText:
//
// Invalid name URI.
//
#define SF_STATUS_INVALID_NAME_URI       ((NTSTATUS)0xE0431BBEL)

//
// MessageId: SF_STATUS_INVALID_PARTITION_KEY
//
// MessageText:
//
// Invalid partition key or ID.
//
#define SF_STATUS_INVALID_PARTITION_KEY  ((NTSTATUS)0xE0431BBFL)

//
// MessageId: SF_STATUS_NAME_ALREADY_EXISTS
//
// MessageText:
//
// Name already exists.
//
#define SF_STATUS_NAME_ALREADY_EXISTS    ((NTSTATUS)0xE0431BC0L)

//
// MessageId: SF_STATUS_NAME_DOES_NOT_EXIST
//
// MessageText:
//
// Name does not exist.
//
#define SF_STATUS_NAME_DOES_NOT_EXIST    ((NTSTATUS)0xE0431BC1L)

//
// MessageId: SF_STATUS_NAME_NOT_EMPTY
//
// MessageText:
//
// Name not empty.
//
#define SF_STATUS_NAME_NOT_EMPTY         ((NTSTATUS)0xE0431BC2L)

//
// MessageId: SF_STATUS_NODE_NOT_FOUND
//
// MessageText:
//
// Node not found.
//
#define SF_STATUS_NODE_NOT_FOUND         ((NTSTATUS)0xE0431BC3L)

//
// MessageId: SF_STATUS_NODE_IS_UP
//
// MessageText:
//
// Node is up.
//
#define SF_STATUS_NODE_IS_UP             ((NTSTATUS)0xE0431BC4L)

//
// MessageId: SF_STATUS_NO_WRITE_QUORUM
//
// MessageText:
//
// he operation failed because a quorum of nodes are not available for this replica set.  Consider retrying once more nodes are up.
//
#define SF_STATUS_NO_WRITE_QUORUM        ((NTSTATUS)0xE0431BC5L)

//
// MessageId: SF_STATUS_NOT_PRIMARY
//
// MessageText:
//
// The operation failed because this node is not the primary replica.  Consider re-resolving the primary and retrying the operation there.
//
#define SF_STATUS_NOT_PRIMARY            ((NTSTATUS)0xE0431BC6L)

//
// MessageId: SF_STATUS_NOT_READY
//
// MessageText:
//
// The requested data is not yet in cache.  This same operation is likely to succeed if retried later.
//
#define SF_STATUS_NOT_READY              ((NTSTATUS)0xE0431BC7L)

//
// MessageId: SF_STATUS_OPERATION_NOT_COMPLETE
//
// MessageText:
//
// The operation has not completed.
//
#define SF_STATUS_OPERATION_NOT_COMPLETE ((NTSTATUS)0xE0431BC8L)

//
// MessageId: SF_STATUS_PROPERTY_DOES_NOT_EXIST
//
// MessageText:
//
// Property does not exist.
//
#define SF_STATUS_PROPERTY_DOES_NOT_EXIST ((NTSTATUS)0xE0431BC9L)

//
// MessageId: SF_STATUS_RECONFIGURATION_PENDING
//
// MessageText:
//
// The operation failed because this replica set is currently reconfiguring.
//
#define SF_STATUS_RECONFIGURATION_PENDING ((NTSTATUS)0xE0431BCAL)

//
// MessageId: SF_STATUS_REPLICATION_QUEUE_FULL
//
// MessageText:
//
// Replication queue is full.
//
#define SF_STATUS_REPLICATION_QUEUE_FULL ((NTSTATUS)0xE0431BCBL)

//
// MessageId: SF_SERVICE_ALREADY_EXISTS
//
// MessageText:
//
// Service already exists.
//
#define SF_SERVICE_ALREADY_EXISTS        ((NTSTATUS)0xE0431BCCL)

//
// MessageId: SF_SERVICE_DOES_NOT_EXIST
//
// MessageText:
//
// Service does not exist.
//
#define SF_SERVICE_DOES_NOT_EXIST        ((NTSTATUS)0xE0431BCDL)

//
// MessageId: SF_STATUS_SERVICE_OFFLINE
//
// MessageText:
//
// The operation failed because the service is currently offline.
//
#define SF_STATUS_SERVICE_OFFLINE        ((NTSTATUS)0xE0431BCEL)

//
// MessageId: SF_STATUS_SERVICE_METADATA_MISMATCH
//
// MessageText:
//
// The operation failed because the service metadata does not match.
//
#define SF_STATUS_SERVICE_METADATA_MISMATCH ((NTSTATUS)0xE0431BCFL)

//
// MessageId: SF_STATUS_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED
//
// MessageText:
//
// The operation failed because creating the service would form an affinity chain which is not supported.
//
#define SF_STATUS_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED ((NTSTATUS)0xE0431BD0L)

//
// MessageId: SF_STATUS_SERVICE_TYPE_ALREADY_REGISTERED
//
// MessageText:
//
// Service Type is already registered.
//
#define SF_STATUS_SERVICE_TYPE_ALREADY_REGISTERED ((NTSTATUS)0xE0431BD1L)

//
// MessageId: SF_STATUS_SERVICE_TYPE_NOT_REGISTERED
//
// MessageText:
//
// Service Type is not registered.
//
#define SF_STATUS_SERVICE_TYPE_NOT_REGISTERED ((NTSTATUS)0xE0431BD2L)

//
// MessageId: SF_STATUS_VALUE_TOO_LARGE
//
// MessageText:
//
// Invalid argument
//
#define SF_STATUS_VALUE_TOO_LARGE        ((NTSTATUS)0xE0431BD3L)

//
// MessageId: SF_STATUS_VALUE_EMPTY
//
// MessageText:
//
// The property value is empty.
//
#define SF_STATUS_VALUE_EMPTY            ((NTSTATUS)0xE0431BD4L)

//
// MessageId: SF_STATUS_PROPERTY_CHECK_FAILED
//
// MessageText:
//
// The property check failed.
//
#define SF_STATUS_PROPERTY_CHECK_FAILED  ((NTSTATUS)0xE0431BD5L)

//
// MessageId: SF_STATUS_WRITE_CONFLICT
//
// MessageText:
//
// Write conflict.
//
#define SF_STATUS_WRITE_CONFLICT         ((NTSTATUS)0xE0431BD6L)

//
// MessageId: SF_STATUS_ENUMERATION_COMPLETED
//
// MessageText:
//
// Enumeration has completed.
//
#define SF_STATUS_ENUMERATION_COMPLETED  ((NTSTATUS)0xE0431BD7L)

//
// MessageId: SF_STATUS_APPLICATION_TYPE_PROVISION_IN_PROGRESS
//
// MessageText:
//
// Cannot provision multiple versions of the same application type simultaneously.
//
#define SF_STATUS_APPLICATION_TYPE_PROVISION_IN_PROGRESS ((NTSTATUS)0xE0431BD8L)

//
// MessageId: SF_STATUS_APPLICATION_TYPE_ALREADY_EXISTS
//
// MessageText:
//
// Application type and version already exists
//
#define SF_STATUS_APPLICATION_TYPE_ALREADY_EXISTS ((NTSTATUS)0xE0431BD9L)

//
// MessageId: SF_STATUS_APPLICATION_TYPE_NOT_FOUND
//
// MessageText:
//
// Application type and version not found
//
#define SF_STATUS_APPLICATION_TYPE_NOT_FOUND ((NTSTATUS)0xE0431BDAL)

//
// MessageId: SF_STATUS_APPLICATION_TYPE_IN_USE
//
// MessageText:
//
// Application type and version is still in use
//
#define SF_STATUS_APPLICATION_TYPE_IN_USE ((NTSTATUS)0xE0431BDBL)

//
// MessageId: SF_STATUS_APPLICATION_ALREADY_EXISTS
//
// MessageText:
//
// APPLICATION_ALREADY_EXISTS 
//
#define SF_STATUS_APPLICATION_ALREADY_EXISTS ((NTSTATUS)0xE0431BDCL)

//
// MessageId: SF_STATUS_APPLICATION_NOT_FOUND
//
// MessageText:
//
// Application not found
//
#define SF_STATUS_APPLICATION_NOT_FOUND  ((NTSTATUS)0xE0431BDDL)

//
// MessageId: SF_STATUS_APPLICATION_UPGRADE_IN_PROGRESS
//
// MessageText:
//
// Application is currently being upgraded
//
#define SF_STATUS_APPLICATION_UPGRADE_IN_PROGRESS ((NTSTATUS)0xE0431BDEL)

//
// MessageId: SF_STATUS_APPLICATION_UPGRADE_VALIDATION_ERROR
//
// MessageText:
//
// Invalid application upgrade request
//
#define SF_STATUS_APPLICATION_UPGRADE_VALIDATION_ERROR ((NTSTATUS)0xE0431BDFL)

//
// MessageId: SF_STATUS_SERVICE_TYPE_NOT_FOUND
//
// MessageText:
//
// Service Type not found
//
#define SF_STATUS_SERVICE_TYPE_NOT_FOUND ((NTSTATUS)0xE0431BE0L)

//
// MessageId: SF_STATUS_SERVICE_TYPE_MISMATCH
//
// MessageText:
//
// Service Type mismatches description type (i.e. stateless/stateful/persisted)
//
#define SF_STATUS_SERVICE_TYPE_MISMATCH  ((NTSTATUS)0xE0431BE1L)

//
// MessageId: SF_STATUS_SERVICE_TYPE_TEMPLATE_NOT_FOUND
//
// MessageText:
//
// Service Type template not found
//
#define SF_STATUS_SERVICE_TYPE_TEMPLATE_NOT_FOUND ((NTSTATUS)0xE0431BE2L)

//
// MessageId: SF_STATUS_CONFIGURATION_SECTION_NOT_FOUND
//
// MessageText:
//
// A configuration section is missing.
//
#define SF_STATUS_CONFIGURATION_SECTION_NOT_FOUND ((NTSTATUS)0xE0431BE3L)

//
// MessageId: SF_STATUS_CONFIGURATION_PARAMETER_NOT_FOUND
//
// MessageText:
//
// A configuration parameter is missing.
//
#define SF_STATUS_CONFIGURATION_PARAMETER_NOT_FOUND ((NTSTATUS)0xE0431BE4L)

//
// MessageId: SF_STATUS_INVALID_CONFIGURATION
//
// MessageText:
//
// A configuration parameter has been set to an invalid value. Please check the logs for details.
//
#define SF_STATUS_INVALID_CONFIGURATION  ((NTSTATUS)0xE0431BE5L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_VALIDATION_ERROR
//
// MessageText:
//
// There is a content validation error in the manifest file(s)
//
#define SF_STATUS_IMAGEBUILDER_VALIDATION_ERROR ((NTSTATUS)0xE0431BE6L)

//
// MessageId: SF_STATUS_PARTITION_NOT_FOUND
//
// MessageText:
//
// The operation failed because the requested partition does not exist.
//
#define SF_STATUS_PARTITION_NOT_FOUND    ((NTSTATUS)0xE0431BE7L)

//
// MessageId: SF_STATUS_REPLICA_DOES_NOT_EXIST
//
// MessageText:
//
// The specified replica does not exist.
//
#define SF_STATUS_REPLICA_DOES_NOT_EXIST ((NTSTATUS)0xE0431BE8L)

//
// MessageId: SF_STATUS_SERVICE_GROUP_ALREADY_EXISTS
//
// MessageText:
//
// Service group already exists.
//
#define SF_STATUS_SERVICE_GROUP_ALREADY_EXISTS ((NTSTATUS)0xE0431BE9L)

//
// MessageId: SF_STATUS_SERVICE_GROUP_DOES_NOT_EXIST
//
// MessageText:
//
// Service group does not exist.
//
#define SF_STATUS_SERVICE_GROUP_DOES_NOT_EXIST ((NTSTATUS)0xE0431BEAL)

//
// MessageId: SF_STATUS_PROCESS_DEACTIVATED
//
// MessageText:
//
// The process has been deactivated.
//
#define SF_STATUS_PROCESS_DEACTIVATED    ((NTSTATUS)0xE0431BEBL)

//
// MessageId: SF_STATUS_PROCESS_ABORTED
//
// MessageText:
//
// The process has aborted.
//
#define SF_STATUS_PROCESS_ABORTED        ((NTSTATUS)0xE0431BECL)

//
// MessageId: SF_STATUS_UPGRADE_FAILED
//
// MessageText:
//
// Fabric upgrade failed on the node.
//
#define SF_STATUS_UPGRADE_FAILED         ((NTSTATUS)0xE04301EDL)

//
// MessageId: SF_STATUS_INVALID_CREDENTIAL_TYPE
//
// MessageText:
//
// The security credential type specified is invalid.
//
#define SF_STATUS_INVALID_CREDENTIAL_TYPE ((NTSTATUS)0xE0431BEEL)

//
// MessageId: SF_STATUS_INVALID_X509_FIND_TYPE
//
// MessageText:
//
// The X509 find type specified is invalid.
//
#define SF_STATUS_INVALID_X509_FIND_TYPE ((NTSTATUS)0xE0431BEFL)

//
// MessageId: SF_STATUS_INVALID_X509_STORE_LOCATION
//
// MessageText:
//
// The X509 store location specified is invalid.
//
#define SF_STATUS_INVALID_X509_STORE_LOCATION ((NTSTATUS)0xE0431BF0L)

//
// MessageId: SF_STATUS_INVALID_X509_STORE_NAME
//
// MessageText:
//
// The X509 store name specified is invalid.
//
#define SF_STATUS_INVALID_X509_STORE_NAME ((NTSTATUS)0xE0431BF1L)

//
// MessageId: SF_STATUS_INVALID_X509_THUMBPRINT
//
// MessageText:
//
// The X509 thumbprint specified is invalid.
//
#define SF_STATUS_INVALID_X509_THUMBPRINT ((NTSTATUS)0xE0431BF2L)

//
// MessageId: SF_STATUS_INVALID_PROTECTION_LEVEL
//
// MessageText:
//
// The protection level specified is invalid.
//
#define SF_STATUS_INVALID_PROTECTION_LEVEL ((NTSTATUS)0xE0431BF3L)

//
// MessageId: SF_STATUS_INVALID_X509_STORE
//
// MessageText:
//
// The X509 store specified is invalid.
//
#define SF_STATUS_INVALID_X509_STORE     ((NTSTATUS)0xE0431BF4L)

//
// MessageId: SF_STATUS_INVALID_SUBJECT_NAME
//
// MessageText:
//
// The X509 subject name specified is invalid.
//
#define SF_STATUS_INVALID_SUBJECT_NAME   ((NTSTATUS)0xE0431BF5L)

//
// MessageId: SF_STATUS_INVALID_ALLOWED_COMMON_NAME_LIST
//
// MessageText:
//
// The specified list of allowed common names is invalid.
//
#define SF_STATUS_INVALID_ALLOWED_COMMON_NAME_LIST ((NTSTATUS)0xE0431BF6L)

//
// MessageId: SF_STATUS_INVALID_CREDENTIALS
//
// MessageText:
//
// The specified credentials are invalid.
//
#define SF_STATUS_INVALID_CREDENTIALS    ((NTSTATUS)0xE0431BF7L)

//
// MessageId: SF_STATUS_DECRYPTION_FAILED
//
// MessageText:
//
// Failed to decrypt the fvalue.
//
#define SF_STATUS_DECRYPTION_FAILED      ((NTSTATUS)0xE0431BF8L)

//
// MessageId: SF_STATUS_CONFIGURATION_PACKAGE_NOT_FOUND
//
// MessageText:
//
// The config package was not found
//
#define SF_STATUS_CONFIGURATION_PACKAGE_NOT_FOUND ((NTSTATUS)0xE0431BF9L)

//
// MessageId: SF_STATUS_DATA_PACKAGE_NOT_FOUND
//
// MessageText:
//
// The data package was not found
//
#define SF_STATUS_DATA_PACKAGE_NOT_FOUND ((NTSTATUS)0xE0431BFAL)

//
// MessageId: SF_STATUS_CODE_PACKAGE_NOT_FOUND
//
// MessageText:
//
// The code package was not found
//
#define SF_STATUS_CODE_PACKAGE_NOT_FOUND ((NTSTATUS)0xE0431BFBL)

//
// MessageId: SF_STATUS_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND
//
// MessageText:
//
// The Endpoint resource was not found
//
#define SF_STATUS_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND ((NTSTATUS)0xE0431BFCL)

//
// MessageId: SF_STATUS_INVALID_OPERATION
//
// MessageText:
//
// The operation is invalid.
//
#define SF_STATUS_INVALID_OPERATION      ((NTSTATUS)0xE0431BFDL)

//
// MessageId: SF_STATUS_OBJECT_CLOSED
//
// MessageText:
//
// The object is closed.
//
#define SF_STATUS_OBJECT_CLOSED          ((NTSTATUS)0xE0431BFEL)

//
// MessageId: SF_STATUS_TIMEOUT
//
// MessageText:
//
// Operation timed out.
//
#define SF_STATUS_TIMEOUT                ((NTSTATUS)0xE0431BFFL)

//
// MessageId: SF_STATUS_FILE_NOT_FOUND
//
// MessageText:
//
// File not found.
//
#define SF_STATUS_FILE_NOT_FOUND         ((NTSTATUS)0xE0431C00L)

//
// MessageId: SF_STATUS_DIRECTORY_NOT_FOUND
//
// MessageText:
//
// Directory not found.
//
#define SF_STATUS_DIRECTORY_NOT_FOUND    ((NTSTATUS)0xE0431C01L)

//
// MessageId: SF_STATUS_INVALID_DIRECTORY
//
// MessageText:
//
// Invalid Directory.
//
#define SF_STATUS_INVALID_DIRECTORY      ((NTSTATUS)0xE0431C02L)

//
// MessageId: SF_STATUS_PATH_TOO_LONG
//
// MessageText:
//
// Path is too long.
//
#define SF_STATUS_PATH_TOO_LONG          ((NTSTATUS)0xE0431C03L)

//
// MessageId: SF_STATUS_IMAGESTORE_IOERROR
//
// MessageText:
//
// There was an IOException when using the Image Store.
//
#define SF_STATUS_IMAGESTORE_IOERROR     ((NTSTATUS)0xE0431C04L)

//
// MessageId: SF_STATUS_CORRUPTED_IMAGE_STORE_OBJECT_FOUND
//
// MessageText:
//
// The Image Store object related to this operation appears corrupted. Please re-upload to the Image Store incoming folder before retrying the operation.
//
#define SF_STATUS_CORRUPTED_IMAGE_STORE_OBJECT_FOUND ((NTSTATUS)0xE0431C05L)

//
// MessageId: SF_STATUS_APPLICATION_NOT_UPGRADING
//
// MessageText:
//
// Application is currently NOT being upgraded
//
#define SF_STATUS_APPLICATION_NOT_UPGRADING ((NTSTATUS)0xE0431C06L)

//
// MessageId: SF_STATUS_APPLICATION_ALREADY_IN_TARGET_VERSION
//
// MessageText:
//
// Application is already in the target version
//
#define SF_STATUS_APPLICATION_ALREADY_IN_TARGET_VERSION ((NTSTATUS)0xE0431C07L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_UNEXPECTED_ERROR
//
// MessageText:
//
// The Image Builder encountered an unexpected error.
//
#define SF_STATUS_IMAGEBUILDER_UNEXPECTED_ERROR ((NTSTATUS)0xE0431C08L)

//
// MessageId: SF_STATUS_FABRIC_VERSION_NOT_FOUND
//
// MessageText:
//
// Fabric version has not been registered
//
#define SF_STATUS_FABRIC_VERSION_NOT_FOUND ((NTSTATUS)0xE0431C09L)

//
// MessageId: SF_STATUS_FABRIC_VERSION_IN_USE
//
// MessageText:
//
// Fabric is currently in this version or being upgraded to this version
//
#define SF_STATUS_FABRIC_VERSION_IN_USE  ((NTSTATUS)0xE0431C0AL)

//
// MessageId: SF_STATUS_FABRIC_VERSION_ALREADY_EXISTS
//
// MessageText:
//
// Version has already been registered
//
#define SF_STATUS_FABRIC_VERSION_ALREADY_EXISTS ((NTSTATUS)0xE0431C0BL)

//
// MessageId: SF_STATUS_FABRIC_ALREADY_IN_TARGET_VERSION
//
// MessageText:
//
// Fabric is already in this version
//
#define SF_STATUS_FABRIC_ALREADY_IN_TARGET_VERSION ((NTSTATUS)0xE0431C0CL)

//
// MessageId: SF_STATUS_FABRIC_NOT_UPGRADING
//
// MessageText:
//
// There is no pending Fabric upgrade
//
#define SF_STATUS_FABRIC_NOT_UPGRADING   ((NTSTATUS)0xE0431C0DL)

//
// MessageId: SF_STATUS_FABRIC_UPGRADE_IN_PROGRESS
//
// MessageText:
//
// Fabric is being upgraded
//
#define SF_STATUS_FABRIC_UPGRADE_IN_PROGRESS ((NTSTATUS)0xE0431C0EL)

//
// MessageId: SF_STATUS_FABRIC_UPGRADE_VALIDATION_ERROR
//
// MessageText:
//
// Fabric upgrade request failed validation
//
#define SF_STATUS_FABRIC_UPGRADE_VALIDATION_ERROR ((NTSTATUS)0xE0431C0FL)

//
// MessageId: SF_STATUS_HEALTH_MAX_REPORTS_REACHED
//
// MessageText:
//
// Max number of health reports reached, try again
//
#define SF_STATUS_HEALTH_MAX_REPORTS_REACHED ((NTSTATUS)0xE0431C10L)

//
// MessageId: SF_STATUS_HEALTH_STALE_REPORT
//
// MessageText:
//
// Health report is stale
//
#define SF_STATUS_HEALTH_STALE_REPORT    ((NTSTATUS)0xE0431C11L)

//
// MessageId: SF_STATUS_KEY_TOO_LARGE
//
// MessageText:
//
// The key is too large.
//
#define SF_STATUS_KEY_TOO_LARGE          ((NTSTATUS)0xE0431C12L)

//
// MessageId: SF_STATUS_KEY_NOT_FOUND
//
// MessageText:
//
// The given key was not present.
//
#define SF_STATUS_KEY_NOT_FOUND          ((NTSTATUS)0xE0431C13L)

//
// MessageId: SF_STATUS_SEQUENCE_NUMBER_CHECK_FAILED
//
// MessageText:
//
// Sequence number check failed
//
#define SF_STATUS_SEQUENCE_NUMBER_CHECK_FAILED ((NTSTATUS)0xE0431C14L)

//
// MessageId: SF_STATUS_ENCRYPTION_FAILED
//
// MessageText:
//
// Failed to encrypt the value.
//
#define SF_STATUS_ENCRYPTION_FAILED      ((NTSTATUS)0xE0431C15L)

//
// MessageId: SF_STATUS_INVALID_ATOMIC_GROUP
//
// MessageText:
//
// The specified atomic group has not been created or no longer exists.
//
#define SF_STATUS_INVALID_ATOMIC_GROUP   ((NTSTATUS)0xE0431C16L)

//
// MessageId: SF_STATUS_HEALTH_ENTITY_NOT_FOUND
//
// MessageText:
//
// Entity not found in Health Store
//
#define SF_STATUS_HEALTH_ENTITY_NOT_FOUND ((NTSTATUS)0xE0431C17L)

//
// MessageId: SF_STATUS_SERVICE_MANIFEST_NOT_FOUND
//
// MessageText:
//
// Service Manifest not found
//
#define SF_STATUS_SERVICE_MANIFEST_NOT_FOUND ((NTSTATUS)0xE0431C18L)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE
//
// MessageText:
//
// Reliable session transport startup failure
//
#define SF_STATUS_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE ((NTSTATUS)0xE0431C19L)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_ALREADY_EXISTS
//
// MessageText:
//
// Reliable session already exists
//
#define SF_STATUS_RELIABLE_SESSION_ALREADY_EXISTS ((NTSTATUS)0xE0431C1AL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_CANNOT_CONNECT
//
// MessageText:
//
// Reliable session cannot connect
//
#define SF_STATUS_RELIABLE_SESSION_CANNOT_CONNECT ((NTSTATUS)0xE0431C1BL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_MANAGER_EXISTS
//
// MessageText:
//
// Reliable session manager exists
//
#define SF_STATUS_RELIABLE_SESSION_MANAGER_EXISTS ((NTSTATUS)0xE0431C1CL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_REJECTED
//
// MessageText:
//
// Reliable session rejected
//
#define SF_STATUS_RELIABLE_SESSION_REJECTED ((NTSTATUS)0xE0431C1DL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING
//
// MessageText:
//
// Reliable session manager already listening
//
#define SF_STATUS_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING ((NTSTATUS)0xE0431C1EL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_FOUND
//
// MessageText:
//
// Reliable session manager not found
//
#define SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_FOUND ((NTSTATUS)0xE0431C1FL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_LISTENING
//
// MessageText:
//
// Reliable session manager not listening
//
#define SF_STATUS_RELIABLE_SESSION_MANAGER_NOT_LISTENING ((NTSTATUS)0xE0431C20L)

//
// MessageId: SF_STATUS_INVALID_SERVICE_TYPE
//
// MessageText:
//
// Invalid Service Type
//
#define SF_STATUS_INVALID_SERVICE_TYPE   ((NTSTATUS)0xE0431C21L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_TIMEOUT
//
// MessageText:
//
// Image Builder timed out. Please check that the Image Store is available and consider providing a larger timeout when processing large application packages.
//
#define SF_STATUS_IMAGEBUILDER_TIMEOUT   ((NTSTATUS)0xE0431C22L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_ACCESS_DENIED
//
// MessageText:
//
// Please check that the Image Store is available and has correct access permissions for Microsoft Azure Service Fabric processes.
//
#define SF_STATUS_IMAGEBUILDER_ACCESS_DENIED ((NTSTATUS)0xE0431C23L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_INVALID_MSI_FILE
//
// MessageText:
//
// The MSI file is invalid.
//
#define SF_STATUS_IMAGEBUILDER_INVALID_MSI_FILE ((NTSTATUS)0xE0431C24L)

//
// MessageId: SF_STATUS_SERVICE_TOO_BUSY
//
// MessageText:
//
// The service cannot process the request because it is too busy. Please retry.
//
#define SF_STATUS_SERVICE_TOO_BUSY       ((NTSTATUS)0xE0431C25L)

//
// MessageId: SF_STATUS_TRANSACTION_NOT_ACTIVE
//
// MessageText:
//
// Transaction has already committed or rolled back
//
#define SF_STATUS_TRANSACTION_NOT_ACTIVE ((NTSTATUS)0xE0431C26L)

//
// MessageId: SF_STATUS_REPAIR_TASK_ALREADY_EXISTS
//
// MessageText:
//
// The repair task already exists.
//
#define SF_STATUS_REPAIR_TASK_ALREADY_EXISTS ((NTSTATUS)0xE0431C27L)

//
// MessageId: SF_STATUS_REPAIR_TASK_NOT_FOUND
//
// MessageText:
//
// The repair task could not be found.
//
#define SF_STATUS_REPAIR_TASK_NOT_FOUND  ((NTSTATUS)0xE0431C28L)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_NOT_FOUND
//
// MessageText:
//
// Reliable session not found
//
#define SF_STATUS_RELIABLE_SESSION_NOT_FOUND ((NTSTATUS)0xE0431C29L)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_QUEUE_EMPTY
//
// MessageText:
//
// Reliable session queue empty
//
#define SF_STATUS_RELIABLE_SESSION_QUEUE_EMPTY ((NTSTATUS)0xE0431C2AL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_QUOTA_EXCEEDED
//
// MessageText:
//
// Reliable session quota exceeded
//
#define SF_STATUS_RELIABLE_SESSION_QUOTA_EXCEEDED ((NTSTATUS)0xE0431C2BL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_SERVICE_FAULTED
//
// MessageText:
//
// Reliable session service faulted
//
#define SF_STATUS_RELIABLE_SESSION_SERVICE_FAULTED ((NTSTATUS)0xE0431C2CL)

//
// MessageId: SF_STATUS_RELIABLE_SESSION_INVALID_TARGET_PARTITION
//
// MessageText:
//
// Reliable session invalid target partition provided
//
#define SF_STATUS_RELIABLE_SESSION_INVALID_TARGET_PARTITION ((NTSTATUS)0xE0431C2DL)

//
// MessageId: SF_STATUS_TRANSACTION_TOO_LARGE
//
// MessageText:
//
// Transaction data exceeds the configured replication message size limit - ReplicatorSettings.MaxReplicationMessageSize
//
#define SF_STATUS_TRANSACTION_TOO_LARGE  ((NTSTATUS)0xE0431C2EL)

//
// MessageId: SF_STATUS_REPLICATION_OPERATION_TOO_LARGE
//
// MessageText:
//
// The replication operation is larger than the configured limit - MaxReplicationMessageSize
//
#define SF_STATUS_REPLICATION_OPERATION_TOO_LARGE ((NTSTATUS)0xE0431C2FL)

//
// MessageId: SF_STATUS_INSTANCE_ID_MISMATCH
//
// MessageText:
//
// The provided InstanceId did not match.
//
#define SF_STATUS_INSTANCE_ID_MISMATCH   ((NTSTATUS)0xE0431C30L)

//
// MessageId: SF_STATUS_UPGRADE_DOMAIN_ALREADY_COMPLETED
//
// MessageText:
//
// Specified upgrade domain has already completed.
//
#define SF_STATUS_UPGRADE_DOMAIN_ALREADY_COMPLETED ((NTSTATUS)0xE0431C31L)

//
// MessageId: SF_STATUS_NODE_HAS_NOT_STOPPED_YET
//
// MessageText:
//
// Node has not stopped yet - a previous StopNode is still pending
//
#define SF_STATUS_NODE_HAS_NOT_STOPPED_YET ((NTSTATUS)0xE0431C32L)

//
// MessageId: SF_STATUS_INSUFFICIENT_CLUSTER_CAPACITY
//
// MessageText:
//
// The cluster did not have enough resources to perform the requested operation.
//
#define SF_STATUS_INSUFFICIENT_CLUSTER_CAPACITY ((NTSTATUS)0xE0431C33L)

//
// MessageId: SF_STATUS_INVALID_PACKAGE_SHARING_POLICY
//
// MessageText:
//
// PackageSharingPolicy must contain a valid PackageName or Scope
//
#define SF_STATUS_INVALID_PACKAGE_SHARING_POLICY ((NTSTATUS)0xE0431C34L)

//
// MessageId: SF_STATUS_PREDEPLOYMENT_NOT_ALLOWED
//
// MessageText:
//
// Service Manifest packages could not be deployed to node because Image Cache is disabled
//
#define SF_STATUS_PREDEPLOYMENT_NOT_ALLOWED ((NTSTATUS)0xE0431C35L)

//
// MessageId: SF_STATUS_INVALID_BACKUP_SETTING
//
// MessageText:
//
// Invalid backup setting. E.g. incremental backup option is not set upfront
//
#define SF_STATUS_INVALID_BACKUP_SETTING ((NTSTATUS)0xE0431C36L)

//
// MessageId: SF_STATUS_MISSING_FULL_BACKUP
//
// MessageText:
//
// Incremental backups can only be done after an initial full backup
//
#define SF_STATUS_MISSING_FULL_BACKUP    ((NTSTATUS)0xE0431C37L)

//
// MessageId: SF_STATUS_BACKUP_IN_PROGRESS
//
// MessageText:
//
// A previously initiated backup is currently in progress
//
#define SF_STATUS_BACKUP_IN_PROGRESS     ((NTSTATUS)0xE0431C38L)

//
// MessageId: SF_STATUS_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME
//
// MessageText:
//
// Service notification filter has already been registered at the specified name.
//
#define SF_STATUS_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME ((NTSTATUS)0xE0431C39L)

//
// MessageId: SF_STATUS_INVALID_REPLICA_OPERATION
//
// MessageText:
//
// Restart can only be performed on persisted services. For volatile or stateless services use Remove.
//
#define SF_STATUS_INVALID_REPLICA_OPERATION ((NTSTATUS)0xE0431C3AL)

//
// MessageId: SF_STATUS_INVALID_REPLICA_STATE
//
// MessageText:
//
// This operation cannot be performed in the current replica state.
//
#define SF_STATUS_INVALID_REPLICA_STATE  ((NTSTATUS)0xE0431C3BL)

//
// MessageId: SF_STATUS_LOADBALANCER_NOT_READY
//
// MessageText:
//
// Load Balancer is currently busy.
//
#define SF_STATUS_LOADBALANCER_NOT_READY ((NTSTATUS)0xE0431C3CL)

//
// MessageId: SF_STATUS_INVALID_PARTITION_OPERATION
//
// MessageText:
//
// MovePrimary or MoveSecondary operation can be only be performed on stateful Service Type.
//
#define SF_STATUS_INVALID_PARTITION_OPERATION ((NTSTATUS)0xE0431C3DL)

//
// MessageId: SF_STATUS_PRIMARY_ALREADY_EXISTS
//
// MessageText:
//
// Replica is already primary role.
//
#define SF_STATUS_PRIMARY_ALREADY_EXISTS ((NTSTATUS)0xE0431C3EL)

//
// MessageId: SF_STATUS_SECONDARY_ALREADY_EXISTS
//
// MessageText:
//
// Replica is already secondary role.
//
#define SF_STATUS_SECONDARY_ALREADY_EXISTS ((NTSTATUS)0xE0431C3FL)

//
// MessageId: SF_STATUS_BACKUP_DIRECTORY_NOT_EMPTY
//
// MessageText:
//
// The backup directory is not empty
//
#define SF_STATUS_BACKUP_DIRECTORY_NOT_EMPTY ((NTSTATUS)0xE0431C40L)

//
// MessageId: SF_STATUS_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION
//
// MessageText:
//
// Replicas hosted in Fabric.exe or in processes not managed by Microsoft Azure Service Fabric cannot be force removed.
//
#define SF_STATUS_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION ((NTSTATUS)0xE0431C41L)

//
// MessageId: SF_STATUS_ACQUIRE_FILE_LOCK_FAILED
//
// MessageText:
//
// There was an error acquiring lock on the file. This indicates that another process has acquired write lock on the file or the process does not have access to the file location.
//
#define SF_STATUS_ACQUIRE_FILE_LOCK_FAILED ((NTSTATUS)0xE0431C42L)

//
// MessageId: SF_STATUS_CONNECTION_DENIED
//
// MessageText:
//
// Not authorized to connect
//
#define SF_STATUS_CONNECTION_DENIED      ((NTSTATUS)0xE0431C43L)

//
// MessageId: SF_STATUS_SERVER_AUTHENTICATION_FAILED
//
// MessageText:
//
// Failed to authenticate server identity
//
#define SF_STATUS_SERVER_AUTHENTICATION_FAILED ((NTSTATUS)0xE0431C44L)

//
// MessageId: SF_STATUS_CONSTRAINT_KEY_UNDEFINED
//
// MessageText:
//
// One or more placement constraints on the service are undefined on all nodes that are currently up.
//
#define SF_STATUS_CONSTRAINT_KEY_UNDEFINED ((NTSTATUS)0xE0431C45L)

//
// MessageId: SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED
//
// MessageText:
//
// Multithreaded usage of transactions is not allowed.
//
#define SF_STATUS_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED ((NTSTATUS)0xE0431C46L)

//
// MessageId: SF_STATUS_INVALID_X509_NAME_LIST
//
// MessageText:
//
// 
//
#define SF_STATUS_INVALID_X509_NAME_LIST ((NTSTATUS)0xE0431C47L)

//
// MessageId: SF_STATUS_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED
//
// MessageText:
//
// 
//
#define SF_STATUS_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED ((NTSTATUS)0xE0431C48L)

//
// MessageId: SF_STATUS_GATEWAY_NOT_REACHABLE
//
// MessageText:
//
// A communication error caused the operation to fail.
//
#define SF_STATUS_GATEWAY_NOT_REACHABLE  ((NTSTATUS)0xE0431C49L)

//
// MessageId: SF_STATUS_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED
//
// MessageText:
//
// Certificate for UserRole FabricClient is not configured.
//
#define SF_STATUS_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED ((NTSTATUS)0xE0431C4AL)

//
// MessageId: SF_STATUS_TRANSACTION_ABORTED
//
// MessageText:
//
// Transaction cannot be used after encountering an error. Retries must occur on a new transaction.
//
#define SF_STATUS_TRANSACTION_ABORTED    ((NTSTATUS)0xE0431C4BL)

//
// MessageId: SF_STATUS_CANNOT_CONNECT
//
// MessageText:
//
// A FabricErrorCode that indicates there was a connection failure.
//
#define SF_STATUS_CANNOT_CONNECT         ((NTSTATUS)0xE0431C4CL)

//
// MessageId: SF_STATUS_MESSAGE_TOO_LARGE
//
// MessageText:
//
// A FabricErrorCode that indicates the message is too large.
//
#define SF_STATUS_MESSAGE_TOO_LARGE      ((NTSTATUS)0xE0431C4DL)

//
// MessageId: SF_STATUS_CONSTRAINT_NOT_SATISFIED
//
// MessageText:
//
// The service's and cluster's configuration settings would result in a constraint-violating state if the operation were executed.
//
#define SF_STATUS_CONSTRAINT_NOT_SATISFIED ((NTSTATUS)0xE0431C4EL)

//
// MessageId: SF_STATUS_ENDPOINT_NOT_FOUND
//
// MessageText:
//
// A FabricErrorCode that indicates the specified endpoint was not found.
//
#define SF_STATUS_ENDPOINT_NOT_FOUND     ((NTSTATUS)0xE0431C4FL)

//
// MessageId: SF_STATUS_APPLICATION_UPDATE_IN_PROGRESS
//
// MessageText:
//
// A FabricErrorCode that indicates the application is currently being updated.
//
#define SF_STATUS_APPLICATION_UPDATE_IN_PROGRESS ((NTSTATUS)0xE0431C50L)

//
// MessageId: SF_STATUS_DELETE_BACKUP_FILE_FAILED
//
// MessageText:
//
// Deletion of backup files/directory failed. Currently this can happen in a scenario where backup is used mainly to truncate logs.
//
#define SF_STATUS_DELETE_BACKUP_FILE_FAILED ((NTSTATUS)0xE0431C51L)

//
// MessageId: SF_STATUS_CONNECTION_CLOSED_BY_REMOTE_END
//
// MessageText:
//
// A FabricErrorCode that indicates the connection was closed by the remote end.
//
#define SF_STATUS_CONNECTION_CLOSED_BY_REMOTE_END ((NTSTATUS)0xE0431C52L)

//
// MessageId: SF_STATUS_INVALID_TEST_COMMAND_STATE
//
// MessageText:
//
// A FabricErrorCode that indicates that this API call is not valid for the current state of the test command.
//
#define SF_STATUS_INVALID_TEST_COMMAND_STATE ((NTSTATUS)0xE0431C53L)

//
// MessageId: SF_STATUS_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS
//
// MessageText:
//
// A FabricErrorCode that indicates that this test command operation id (Guid) is already being used.
//
#define SF_STATUS_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS ((NTSTATUS)0xE0431C54L)

//
// MessageId: SF_STATUS_CM_OPERATION_FAILED
//
// MessageText:
//
// Creation or deletion terminated due to persistent failures after bounded retry.
//
#define SF_STATUS_CM_OPERATION_FAILED    ((NTSTATUS)0xE0431C55L)

//
// MessageId: SF_STATUS_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR
//
// MessageText:
//
// A FabricErrorCode that indicates the path passed by the user starts with a reserved directory.
//
#define SF_STATUS_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR ((NTSTATUS)0xE0431C56L)

//
// MessageId: SF_STATUS_CERTIFICATE_NOT_FOUND
//
// MessageText:
//
// A FabricErrorCode that indicates the certificate is not found.
//
#define SF_STATUS_CERTIFICATE_NOT_FOUND  ((NTSTATUS)0xE0431C57L)

//
// MessageId: SF_STATUS_CHAOS_ALREADY_RUNNING
//
// MessageText:
//
// A FabricErrorCode that indicates that an instance of Chaos is already running.
//
#define SF_STATUS_CHAOS_ALREADY_RUNNING  ((NTSTATUS)0xE0431C58L)

//
// MessageId: SF_STATUS_FABRIC_DATA_ROOT_NOT_FOUND
//
// MessageText:
//
// Fabric Data Root is not defined on the target machine.
//
#define SF_STATUS_FABRIC_DATA_ROOT_NOT_FOUND ((NTSTATUS)0xE0431C59L)

//
// MessageId: SF_STATUS_INVALID_RESTORE_DATA
//
// MessageText:
//
// Indicates that restore metadata present in supplied restore directory in invalid.
//
#define SF_STATUS_INVALID_RESTORE_DATA   ((NTSTATUS)0xE0431C5AL)

//
// MessageId: SF_STATUS_DUPLICATE_BACKUPS
//
// MessageText:
//
// Indicates that backup-chain in specified restore directory contains duplicate backups.
//
#define SF_STATUS_DUPLICATE_BACKUPS      ((NTSTATUS)0xE0431C5BL)

//
// MessageId: SF_STATUS_INVALID_BACKUP_CHAIN
//
// MessageText:
//
// Indicates that backup-chain in specified restore directory has one or more missing backups.
//
#define SF_STATUS_INVALID_BACKUP_CHAIN   ((NTSTATUS)0xE0431C5CL)

//
// MessageId: SF_STATUS_STOP_IN_PROGRESS
//
// MessageText:
//
// An operation is already in progress.
//
#define SF_STATUS_STOP_IN_PROGRESS       ((NTSTATUS)0xE0431C5DL)

//
// MessageId: SF_STATUS_ALREADY_STOPPED
//
// MessageText:
//
// The node is already in a stopped state.
//
#define SF_STATUS_ALREADY_STOPPED        ((NTSTATUS)0xE0431C5EL)

//
// MessageId: SF_STATUS_NODE_IS_DOWN
//
// MessageText:
//
// The node is down (not stopped).
//
#define SF_STATUS_NODE_IS_DOWN           ((NTSTATUS)0xE0431C5FL)

//
// MessageId: SF_STATUS_NODE_TRANSITION_IN_PROGRESS
//
// MessageText:
//
// Node transition in progress.
//
#define SF_STATUS_NODE_TRANSITION_IN_PROGRESS ((NTSTATUS)0xE0431C60L)

//
// MessageId: SF_STATUS_INVALID_BACKUP
//
// MessageText:
//
// Indicates that backup provided for restore is invalid.
//
#define SF_STATUS_INVALID_BACKUP         ((NTSTATUS)0xE0431C61L)

//
// MessageId: SF_STATUS_INVALID_INSTANCE_ID
//
// MessageText:
//
// The provided instance id is invalid.
//
#define SF_STATUS_INVALID_INSTANCE_ID    ((NTSTATUS)0xE0431C62L)

//
// MessageId: SF_STATUS_INVALID_DURATION
//
// MessageText:
//
// The provided duration is invalid.
//
#define SF_STATUS_INVALID_DURATION       ((NTSTATUS)0xE0431C63L)

//
// MessageId: SF_STATUS_RESTORE_SAFE_CHECK_FAILED
//
// MessageText:
//
// Indicates that backup provided for restore has older data than present in service.
//
#define SF_STATUS_RESTORE_SAFE_CHECK_FAILED ((NTSTATUS)0xE0431C64L)

//
// MessageId: SF_STATUS_CONFIG_UPGRADE_FAILED
//
// MessageText:
//
// Indicates that the config upgrade fails.
//
#define SF_STATUS_CONFIG_UPGRADE_FAILED  ((NTSTATUS)0xE0431C65L)

//
// MessageId: SF_STATUS_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE
//
// MessageText:
//
// Indicates that the upload session range will overlap or are out of range.
//
#define SF_STATUS_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE ((NTSTATUS)0xE0431C66L)

//
// MessageId: SF_STATUS_UPLOAD_SESSION_ID_CONFLICT
//
// MessageText:
//
// Indicates that the upload session ID is existed for a different image store relative path.
//
#define SF_STATUS_UPLOAD_SESSION_ID_CONFLICT ((NTSTATUS)0xE0431C67L)

//
// MessageId: SF_STATUS_INVALID_PARTITION_SELECTOR
//
// MessageText:
//
// Indicates that the partition selector is invalid.
//
#define SF_STATUS_INVALID_PARTITION_SELECTOR ((NTSTATUS)0xE0431C68L)

//
// MessageId: SF_STATUS_INVALID_REPLICA_SELECTOR
//
// MessageText:
//
// Indicates that the replica selector is invalid.
//
#define SF_STATUS_INVALID_REPLICA_SELECTOR ((NTSTATUS)0xE0431C69L)

//
// MessageId: SF_STATUS_DNS_SERVICE_NOT_FOUND
//
// MessageText:
//
// Indicates that DnsService is not enabled on the cluster.
//
#define SF_STATUS_DNS_SERVICE_NOT_FOUND  ((NTSTATUS)0xE0431C6AL)

//
// MessageId: SF_STATUS_INVALID_DNS_NAME
//
// MessageText:
//
// Indicates that service DNS name is invalid.
//
#define SF_STATUS_INVALID_DNS_NAME       ((NTSTATUS)0xE0431C6BL)

//
// MessageId: SF_STATUS_DNS_NAME_IN_USE
//
// MessageText:
//
// Indicates that service DNS name is in use by another service.
//
#define SF_STATUS_DNS_NAME_IN_USE        ((NTSTATUS)0xE0431C6CL)

//
// MessageId: SF_STATUS_COMPOSE_DEPLOYMENT_ALREADY_EXISTS
//
// MessageText:
//
// Indicates the compose application already exists.
//
#define SF_STATUS_COMPOSE_DEPLOYMENT_ALREADY_EXISTS ((NTSTATUS)0xE0431C6DL)

//
// MessageId: SF_STATUS_COMPOSE_DEPLOYMENT_NOT_FOUND
//
// MessageText:
//
// Indicates the compose application is not found.
//
#define SF_STATUS_COMPOSE_DEPLOYMENT_NOT_FOUND ((NTSTATUS)0xE0431C6EL)

//
// MessageId: SF_STATUS_INVALID_FOR_STATEFUL_SERVICES
//
// MessageText:
//
// Indicates the operation is only valid for stateless services.
//
#define SF_STATUS_INVALID_FOR_STATEFUL_SERVICES ((NTSTATUS)0xE0431C6FL)

//
// MessageId: SF_STATUS_INVALID_FOR_STATELESS_SERVICES
//
// MessageText:
//
// Indicates the operation is only valid for stateful services.
//
#define SF_STATUS_INVALID_FOR_STATELESS_SERVICES ((NTSTATUS)0xE0431C70L)

//
// MessageId: SF_STATUS_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES
//
// MessageText:
//
// Indicates the operation is only valid for stateful persistent services.
//
#define SF_STATUS_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES ((NTSTATUS)0xE0431C71L)

//
// MessageId: SF_STATUS_INVALID_UPLOAD_SESSION_ID
//
// MessageText:
//
// Indicates the upload session ID is invalid. Please use GUID as upload session ID.
//
#define SF_STATUS_INVALID_UPLOAD_SESSION_ID ((NTSTATUS)0xE0431C72L)

//
// MessageId: SF_STATUS_BACKUP_NOT_ENABLED
//
// MessageText:
//
// Indicates that the backup protection is not enabled.
//
#define SF_STATUS_BACKUP_NOT_ENABLED     ((NTSTATUS)0xE0431C73L)

//
// MessageId: SF_STATUS_BACKUP_IS_ENABLED
//
// MessageText:
//
// Indicates that the backup protection is enabled.
//
#define SF_STATUS_BACKUP_IS_ENABLED      ((NTSTATUS)0xE0431C74L)

//
// MessageId: SF_STATUS_BACKUP_POLICY_DOES_NOT_EXIST
//
// MessageText:
//
// Indicates the Backup Policy does not exist.
//
#define SF_STATUS_BACKUP_POLICY_DOES_NOT_EXIST ((NTSTATUS)0xE0431C75L)

//
// MessageId: SF_STATUS_BACKUP_POLICY_ALREADY_EXISTS
//
// MessageText:
//
// Indicates the Backup Policy is already exists.
//
#define SF_STATUS_BACKUP_POLICY_ALREADY_EXISTS ((NTSTATUS)0xE0431C76L)

//
// MessageId: SF_STATUS_RESTORE_IN_PROGRESS
//
// MessageText:
//
// Indicates that a partition is already has a restore in progress.
//
#define SF_STATUS_RESTORE_IN_PROGRESS    ((NTSTATUS)0xE0431C77L)

//
// MessageId: SF_STATUS_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH
//
// MessageText:
//
// Indicates that the source from where restore is requested has a properties mismatch with target partition.
//
#define SF_STATUS_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH ((NTSTATUS)0xE0431C78L)

//
// MessageId: SF_STATUS_FAULT_ANALYSIS_SERVICE_NOT_ENABLED
//
// MessageText:
//
// Indicates the Restore cannot be triggered as Fault Analysis Service is not running.
//
#define SF_STATUS_FAULT_ANALYSIS_SERVICE_NOT_ENABLED ((NTSTATUS)0xE0431C79L)

//
// MessageId: SF_STATUS_CONTAINER_NOT_FOUND
//
// MessageText:
//
// Indicates the container is not found.
//
#define SF_STATUS_CONTAINER_NOT_FOUND    ((NTSTATUS)0xE0431C7AL)

//
// MessageId: SF_STATUS_OBJECT_DISPOSED
//
// MessageText:
//
// The operation is performed on a disposed object.
//
#define SF_STATUS_OBJECT_DISPOSED        ((NTSTATUS)0xE0431C7BL)

//
// MessageId: SF_STATUS_NOT_READABLE
//
// MessageText:
//
// Indicates the partition is not readable.
//
#define SF_STATUS_NOT_READABLE           ((NTSTATUS)0xE0431C7CL)

//
// MessageId: SF_STATUS_BACKUPCOPIER_UNEXPECTED_ERROR
//
// MessageText:
//
// Indicates that the backup copier failed due to unexpected error during its operation.
//
#define SF_STATUS_BACKUPCOPIER_UNEXPECTED_ERROR ((NTSTATUS)0xE0431C7DL)

//
// MessageId: SF_STATUS_BACKUPCOPIER_TIMEOUT
//
// MessageText:
//
// Indicates that the backup copier failed due to timeout during its operation.
//
#define SF_STATUS_BACKUPCOPIER_TIMEOUT   ((NTSTATUS)0xE0431C7EL)

//
// MessageId: SF_STATUS_BACKUPCOPIER_ACCESS_DENIED
//
// MessageText:
//
// Indicates that the backup copier was denied required access to complete operation.
//
#define SF_STATUS_BACKUPCOPIER_ACCESS_DENIED ((NTSTATUS)0xE0431C7FL)

//
// MessageId: SF_STATUS_INVALID_SERVICE_SCALING_POLICY
//
// MessageText:
//
// Indicates that the scaling policy specified for the service is invalid
//
#define SF_STATUS_INVALID_SERVICE_SCALING_POLICY ((NTSTATUS)0xE0431C80L)

//
// MessageId: SF_STATUS_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS
//
// MessageText:
//
// Indicates that the single instance application already exists.
//
#define SF_STATUS_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS ((NTSTATUS)0xE0431C81L)

//
// MessageId: SF_STATUS_SINGLE_INSTANCE_APPLICATION_NOT_FOUND
//
// MessageText:
//
// Indicates that the single instance application is not found.
//
#define SF_STATUS_SINGLE_INSTANCE_APPLICATION_NOT_FOUND ((NTSTATUS)0xE0431C82L)

//
// MessageId: SF_STATUS_VOLUME_ALREADY_EXISTS
//
// MessageText:
//
// Volume already exists.
//
#define SF_STATUS_VOLUME_ALREADY_EXISTS  ((NTSTATUS)0xE0431C83L)

//
// MessageId: SF_STATUS_VOLUME_NOT_FOUND
//
// MessageText:
//
// Indicates that the specified volume is not found.
//
#define SF_STATUS_VOLUME_NOT_FOUND       ((NTSTATUS)0xE0431C84L)

//
// MessageId: SF_STATUS_DATABASE_MIGRATION_IN_PROGRESS
//
// MessageText:
//
// Indicates that the service is undergoing database migration and unavailable for writes
//
#define SF_STATUS_DATABASE_MIGRATION_IN_PROGRESS ((NTSTATUS)0xE0431C85L)

//
// MessageId: SF_STATUS_CENTRAL_SECRET_SERVICE_GENERIC
//
// MessageText:
//
// Central Secret Service generic error.
//
#define SF_STATUS_CENTRAL_SECRET_SERVICE_GENERIC ((NTSTATUS)0xE0431C86L)

//
// MessageId: SF_STATUS_SECRET_INVALID
//
// MessageText:
//
// Invalid secret error.
//
#define SF_STATUS_SECRET_INVALID         ((NTSTATUS)0xE0431C87L)

//
// MessageId: SF_STATUS_SECRET_VERSION_ALREADY_EXISTS
//
// MessageText:
//
// The secret version already exists.
//
#define SF_STATUS_SECRET_VERSION_ALREADY_EXISTS ((NTSTATUS)0xE0431C88L)

//
// MessageId: SF_STATUS_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS
//
// MessageText:
//
// Single instance application is currently being upgraded
//
#define SF_STATUS_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS ((NTSTATUS)0xE0431C89L)

//
// MessageId: SF_STATUS_COMPOSE_DEPLOYMENT_NOT_UPGRADING
//
// MessageText:
//
// Indicates the compose application is not upgrading.
//
#define SF_STATUS_COMPOSE_DEPLOYMENT_NOT_UPGRADING ((NTSTATUS)0xE0431C8AL)

//
// MessageId: SF_STATUS_SECRET_TYPE_CANNOT_BE_CHANGED
//
// MessageText:
//
// The type of an existing secret cannot be changed.
//
#define SF_STATUS_SECRET_TYPE_CANNOT_BE_CHANGED ((NTSTATUS)0xE0431C8BL)

