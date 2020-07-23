// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace ErrorCodeValue
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                // *** common HRESULTs

                // case Success
            case S_OK : w << "S_OK"; return;
                // case NotImplemented
            case E_NOTIMPL : w << "E_NOTIMPL"; return;
                //case E_NOINTERFACE : w << "E_NOINTERFACE";
            case E_POINTER : w << "E_POINTER"; return;
            case E_ABORT : w << "E_ABORT"; return;
            case E_FAIL : w << "E_FAIL"; return;
                //case E_UNEXPECTED : w << "E_UNEXPECTED";
            case E_ACCESSDENIED : w << "E_ACCESSDENIED"; return;
                //case E_HANDLE : w << "E_HANDLE";
            case E_OUTOFMEMORY : w << "E_OUTOFMEMORY"; return;
                // case REInvalidEpoch: w << "REInvalidEpoch";
            case E_INVALIDARG : w << "E_INVALIDARG"; return;

                // *** fabric HRESULTs

                // These are the ErrorCodeValues that share values, since they translate to the
                // same public HRESULT.  To clarify, the public HRESULT value is used instead
                // when multiple identifiers map to that HRESULT.

                // case InvalidState: w << "InvalidState";
                // case REReplicaAlreadyExists: w << "REReplicaAlreadyExists";
            case (int)FABRIC_E_INVALID_OPERATION: w << "FABRIC_E_INVALID_OPERATION"; return;

                // case ObjectClosed: w << "ObjectClosed";
                // case FabricComponentAborted: w << "FabricComponentAborted";
            case (int)FABRIC_E_OBJECT_CLOSED: w << "FABRIC_E_OBJECT_CLOSED"; return;

                // case InvalidMessage: w << "InvalidMessage";
                // case SqlStoreUnableToConnect: w << "SqlStoreUnableToConnect";
                // case InvalidNamingAction: w << "InvalidNamingAction";
                // case UnsupportedNamingOperation: w << "UnsupportedNamingOperation";
            case (int)FABRIC_E_COMMUNICATION_ERROR: w << "FABRIC_E_COMMUNICATION_ERROR"; return;

            case (int)FABRIC_E_INVALID_ADDRESS: w << "FABRIC_E_INVALID_ADDRESS"; return;

            case (int)FABRIC_E_INVALID_NAME_URI: w << "FABRIC_E_INVALID_NAME_URI"; return;

            case (int)FABRIC_E_INVALID_PARTITION_KEY: w << "FABRIC_E_INVALID_PARTITION_KEY"; return;

            case (int)FABRIC_E_NAME_ALREADY_EXISTS: w << "FABRIC_E_NAME_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_NAME_DOES_NOT_EXIST: w << "FABRIC_E_NAME_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_NAME_NOT_EMPTY: w << "FABRIC_E_NAME_NOT_EMPTY"; return;

            case (int)FABRIC_E_NO_WRITE_QUORUM: w << "FABRIC_E_NO_WRITE_QUORUM"; return;

            case (int)FABRIC_E_NOT_PRIMARY: w << "FABRIC_E_NOT_PRIMARY"; return;

            case (int)FABRIC_E_NOT_READY: w << "FABRIC_E_NOT_READY"; return;

            case (int)FABRIC_E_OPERATION_NOT_COMPLETE: w << "FABRIC_E_OPERATION_NOT_COMPLETE"; return;

            case (int)FABRIC_E_PROPERTY_DOES_NOT_EXIST: w << "FABRIC_E_PROPERTY_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_RECONFIGURATION_PENDING: w << "FABRIC_E_RECONFIGURATION_PENDING"; return;

            case (int)FABRIC_E_REPLICATION_QUEUE_FULL: w << "FABRIC_E_REPLICATION_QUEUE_FULL"; return;

            case (int)FABRIC_E_SERVICE_ALREADY_EXISTS: w << "FABRIC_E_SERVICE_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_SERVICE_DOES_NOT_EXIST: w << "FABRIC_E_SERVICE_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_DECRYPTION_FAILED: w << "FABRIC_E_DECRYPTION_FAILED"; return;

            case (int)FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND: w << "FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND"; return;

            case (int)FABRIC_E_DATA_PACKAGE_NOT_FOUND: w << "FABRIC_E_DATA_PACKAGE_NOT_FOUND"; return;

            case (int)FABRIC_E_CODE_PACKAGE_NOT_FOUND: w << "FABRIC_E_CODE_PACKAGE_NOT_FOUND"; return;

            case (int)FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND: w << "FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND"; return;

            case (int)FABRIC_E_TIMEOUT: w << "FABRIC_E_TIMEOUT"; return;

                // case StoreBufferTruncated: w << "StoreBufferTruncated";
                // case PropertyTooLarge: w << "PropertyTooLarge";
            case (int)FABRIC_E_VALUE_TOO_LARGE: w << "FABRIC_E_VALUE_TOO_LARGE"; return;

            case (int)FABRIC_E_VALUE_EMPTY: w << "FABRIC_E_VALUE_EMPTY"; return;

            case (int)FABRIC_E_PROPERTY_CHECK_FAILED: w << "FABRIC_E_PROPERTY_CHECK_FAILED"; return;

            case (int)FABRIC_E_NODE_NOT_FOUND: w << "FABRIC_E_NODE_NOT_FOUND"; return;

            case (int)FABRIC_E_NODE_IS_UP: w << "FABRIC_E_NODE_IS_UP"; return;

            case (int)FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED: w << "FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED"; return;

            case (int)FABRIC_E_SERVICE_TYPE_NOT_REGISTERED: w << "FABRIC_E_SERVICE_TYPE_NOT_REGISTERED"; return;

                // case StoreWriteConflict:   w << "StoreWriteConflict";
                // case StoreRecordNotFound: w << "StoreRecordNotFound";
                // case StoreRecordAlreadyExists: w << "StoreRecordAlreadyExists";
                // case SqlStoreDuplicateInsert: w << "SqlStoreDuplicateInsert";
            case (int)FABRIC_E_WRITE_CONFLICT: w << "FABRIC_E_WRITE_CONFLICT"; return;

                // case ServiceOffline: w << "ServiceOffline";
            case (int)FABRIC_E_SERVICE_OFFLINE: w << "FABRIC_E_SERVICE_OFFLINE"; return;

            case (int)FABRIC_E_SERVICE_METADATA_MISMATCH: w << "FABRIC_E_SERVICE_METADATA_MISMATCH"; return;

            case (int)FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED: w << "FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED"; return;

            case (int)FABRIC_E_ENUMERATION_COMPLETED: w << "FABRIC_E_ENUMERATION_COMPLETED"; return;

            case (int)FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS: w << "FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS"; return;

            case (int)FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS: w << "FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_APPLICATION_TYPE_NOT_FOUND: w << "FABRIC_E_APPLICATION_TYPE_NOT_FOUND"; return;

            case (int)FABRIC_E_APPLICATION_TYPE_IN_USE: w << "FABRIC_E_APPLICATION_TYPE_IN_USE"; return;

            case (int)FABRIC_E_APPLICATION_ALREADY_EXISTS: w << "FABRIC_E_APPLICATION_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_APPLICATION_NOT_FOUND: w << "FABRIC_E_APPLICATION_NOT_FOUND"; return;

            case (int)FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS: w << "FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS"; return;

            case (int)FABRIC_E_APPLICATION_NOT_UPGRADING: w << "FABRIC_E_APPLICATION_NOT_UPGRADING"; return;

            case (int)FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION: w << "FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION"; return;

            case (int)FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR: w << "FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR"; return;

            case (int)FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS: w << "FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS"; return;

            case (int)FABRIC_E_SERVICE_TYPE_NOT_FOUND: w << "FABRIC_E_SERVICE_TYPE_NOT_FOUND"; return;

            case (int)FABRIC_E_SERVICE_TYPE_MISMATCH: w << "FABRIC_E_SERVICE_TYPE_MISMATCH"; return;

            case (int)FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND: w << "FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND"; return;

            case (int)FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND: w << "FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND"; return;

            case (int)FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND: w << "FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND"; return;

            case (int)FABRIC_E_INVALID_CONFIGURATION: w << "FABRIC_E_INVALID_CONFIGURATION"; return;

            case (int)FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR: w << "FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR"; return;

            case (int)FABRIC_E_FILE_NOT_FOUND: w << "FABRIC_E_FILE_NOT_FOUND"; return;

            case (int)FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND: w << "FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND"; return;

            case (int)FABRIC_E_DIRECTORY_NOT_FOUND: w << "FABRIC_E_DIRECTORY_NOT_FOUND"; return;

            case (int)FABRIC_E_INVALID_DIRECTORY: w << "FABRIC_E_INVALID_DIRECTORY"; return;

            case (int)FABRIC_E_PATH_TOO_LONG: w << "FABRIC_E_PATH_TOO_LONG"; return;

            case (int)FABRIC_E_IMAGESTORE_IOERROR: w << "FABRIC_E_IMAGESTORE_IOERROR"; return;

            case (int)FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR: w << "FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR"; return;

            case (int)FABRIC_E_PARTITION_NOT_FOUND: w << "FABRIC_E_PARTITION_NOT_FOUND"; return;

            case (int)FABRIC_E_REPLICA_DOES_NOT_EXIST: w << "FABRIC_E_REPLICA_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS: w << "FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST: w << "FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_PROCESS_DEACTIVATED: w << "FABRIC_E_PROCESS_DEACTIVATED"; return;

            case (int)FABRIC_E_PROCESS_ABORTED: w << "FABRIC_E_PROCESS_ABORTED"; return;

            case (int)FABRIC_E_UPGRADE_FAILED: w << "FABRIC_E_UPGRADE_FAILED"; return;

            case (int)FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED: w << "FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED"; return;

            case (int)FABRIC_E_INVALID_CREDENTIAL_TYPE : w << "FABRIC_E_INVALID_CREDENTIAL_TYPE"; return;

            case (int)FABRIC_E_INVALID_X509_FIND_TYPE : w << "FABRIC_E_INVALID_X509_FIND_TYPE"; return;

            case (int)FABRIC_E_INVALID_X509_STORE_LOCATION : w << "FABRIC_E_INVALID_X509_STORE_LOCATION"; return;

            case (int)FABRIC_E_INVALID_X509_STORE_NAME : w << "FABRIC_E_INVALID_X509_STORE_NAME"; return;

            case (int)FABRIC_E_INVALID_X509_THUMBPRINT : w << "FABRIC_E_INVALID_X509_THUMBPRINT"; return;

            case (int)FABRIC_E_INVALID_X509_NAME_LIST: w << "FABRIC_E_INVALID_X509_NAME_LIST"; return;

            case (int)FABRIC_E_INVALID_PROTECTION_LEVEL : w << "FABRIC_E_INVALID_PROTECTION_LEVEL"; return;

            case (int)FABRIC_E_INVALID_X509_STORE : w << "FABRIC_E_INVALID_X509_STORE"; return;

            case (int)FABRIC_E_INVALID_SUBJECT_NAME : w << "FABRIC_E_INVALID_SUBJECT_NAME"; return;

            case (int)FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST : w << "FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST"; return;

            case (int)FABRIC_E_INVALID_CREDENTIALS : w << "FABRIC_E_INVALID_CREDENTIALS"; return;

            case (int)FABRIC_E_FABRIC_VERSION_NOT_FOUND : w << "FABRIC_E_FABRIC_VERSION_NOT_FOUND"; return;

            case (int)FABRIC_E_FABRIC_VERSION_IN_USE : w << "FABRIC_E_FABRIC_VERSION_IN_USE"; return;

            case (int)FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS : w << "FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION : w << "FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION"; return;

            case (int)FABRIC_E_FABRIC_NOT_UPGRADING : w << "FABRIC_E_FABRIC_NOT_UPGRADING"; return;

            case (int)FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS : w << "FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS"; return;

            case (int)FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR : w << "FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR"; return;

            case (int)FABRIC_E_HEALTH_MAX_REPORTS_REACHED : w << "FABRIC_E_HEALTH_MAX_REPORTS_REACHED"; return;

            case (int)FABRIC_E_HEALTH_STALE_REPORT : w << "FABRIC_E_HEALTH_STALE_REPORT"; return;

            case (int)FABRIC_E_ENCRYPTION_FAILED: w << "FABRIC_E_ENCRYPTION_FAILED"; return;

            case (int)FABRIC_E_INVALID_ATOMIC_GROUP: w << "FABRIC_E_INVALID_ATOMIC_GROUP"; return;

            case (int)FABRIC_E_HEALTH_ENTITY_NOT_FOUND: w << "FABRIC_E_HEALTH_ENTITY_NOT_FOUND"; return;

            case (int)FABRIC_E_SERVICE_MANIFEST_NOT_FOUND: w << "FABRIC_E_SERVICE_MANIFEST_NOT_FOUND"; return;

            case (int)FABRIC_E_KEY_TOO_LARGE: w << "FABRIC_E_KEY_TOO_LARGE"; return;

            case (int)FABRIC_E_KEY_NOT_FOUND: w << "FABRIC_E_KEY_NOT_FOUND"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE: w << "FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS: w << "FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT: w << "FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS: w << "FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_REJECTED: w << "FABRIC_E_RELIABLE_SESSION_REJECTED"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_NOT_FOUND: w << "FABRIC_E_RELIABLE_SESSION_NOT_FOUND"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY: w << "FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED: w << "FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED: w << "FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING: w << "FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND: w << "FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING: w << "FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING"; return;

            case (int)FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION: w << "FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION"; return;

            case (int)FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED: w << "FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED"; return;

            case (int)FABRIC_E_INVALID_SERVICE_TYPE : w << "FABRIC_E_INVALID_SERVICE_TYPE"; return;

            case (int)FABRIC_E_IMAGEBUILDER_TIMEOUT : w << "FABRIC_E_IMAGEBUILDER_TIMEOUT"; return;

            case (int)FABRIC_E_IMAGEBUILDER_ACCESS_DENIED : w << "FABRIC_E_IMAGEBUILDER_ACCESS_DENIED"; return;

            case (int)FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE : w << "FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE"; return;

            case (int)FABRIC_E_SERVICE_TOO_BUSY : w << "FABRIC_E_SERVICE_TOO_BUSY"; return;

            case (int)FABRIC_E_TRANSACTION_NOT_ACTIVE : w << "FABRIC_E_TRANSACTION_NOT_ACTIVE"; return;

            case (int)FABRIC_E_TRANSACTION_TOO_LARGE : w << "FABRIC_E_TRANSACTION_TOO_LARGE"; return;

            case (int)FABRIC_E_REPAIR_TASK_ALREADY_EXISTS : w << "FABRIC_E_REPAIR_TASK_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_REPAIR_TASK_NOT_FOUND : w << "FABRIC_E_REPAIR_TASK_NOT_FOUND"; return;

            case (int)FABRIC_E_REPLICATION_OPERATION_TOO_LARGE : w << "FABRIC_E_REPLICATION_OPERATION_TOO_LARGE"; return;

            case (int)FABRIC_E_INSTANCE_ID_MISMATCH : w << "FABRIC_E_INSTANCE_ID_MISMATCH"; return;

            case (int)FABRIC_E_NODE_HAS_NOT_STOPPED_YET : w << "FABRIC_E_NODE_HAS_NOT_STOPPED_YET"; return;

            case (int)FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY : w << "FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY"; return;

            case (int)FABRIC_E_CONSTRAINT_KEY_UNDEFINED : w << "FABRIC_E_CONSTRAINT_KEY_UNDEFINED"; return;

            case (int)FABRIC_E_CONSTRAINT_NOT_SATISFIED : w << "FABRIC_E_CONSTRAINT_NOT_SATISFIED"; return;

            case (int)FABRIC_E_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED:  w << "FABRIC_E_VERBOSE_FM_PLACEMENT_HEALTH_REPORTING_REQUIRED"; return;

            case (int)FABRIC_E_INVALID_PACKAGE_SHARING_POLICY : w << "FABRIC_E_INVALID_PACKAGE_SHARING_POLICY"; return;

            case (int)FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED : w << "FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED"; return;

            case (int)FABRIC_E_INVALID_BACKUP_SETTING: w << "FABRIC_E_INVALID_BACKUP_SETTING"; return;

            case (int)FABRIC_E_INVALID_RESTORE_DATA: w << "FABRIC_E_INVALID_RESTORE_DATA"; return;

            case (int)FABRIC_E_RESTORE_SAFE_CHECK_FAILED: w << "FABRIC_E_RESTORE_SAFE_CHECK_FAILED"; return;

            case (int)FABRIC_E_DUPLICATE_BACKUPS: w << "FABRIC_E_DUPLICATE_BACKUPS"; return;

            case (int)FABRIC_E_INVALID_BACKUP_CHAIN: w << "FABRIC_E_INVALID_BACKUP_CHAIN"; return;

            case (int)FABRIC_E_INVALID_BACKUP: w << "FABRIC_E_INVALID_BACKUP"; return;

            case (int)FABRIC_E_MISSING_FULL_BACKUP: w << "FABRIC_E_MISSING_FULL_BACKUP"; return;

            case (int)FABRIC_E_BACKUP_IN_PROGRESS: w << "FABRIC_E_BACKUP_IN_PROGRESS"; return;

            case (int)FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY: w << "FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY"; return;

            case (int)FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME: w << "FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME"; return;

            case (int)FABRIC_E_INVALID_REPLICA_OPERATION: w << "FABRIC_E_INVALID_REPLICA_OPERATION"; return;

            case (int)FABRIC_E_INVALID_REPLICA_STATE: w << "FABRIC_E_INVALID_REPLICA_STATE"; return;

            case (int)FABRIC_E_LOADBALANCER_NOT_READY: w << "FABRIC_E_LOADBALANCER_NOT_READY"; return;

            case (int)FABRIC_E_INVALID_PARTITION_OPERATION: w << "FABRIC_E_INVALID_PARTITION_OPERATION"; return;
            case (int)FABRIC_E_PRIMARY_ALREADY_EXISTS: w << "FABRIC_E_PRIMARY_ALREADY_EXISTS"; return;
            case (int)FABRIC_E_SECONDARY_ALREADY_EXISTS: w << "FABRIC_E_SECONDARY_ALREADY_EXISTS"; return;
            case (int)FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION: w << "FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION"; return;
            case (int)FABRIC_E_ACQUIRE_FILE_LOCK_FAILED: w << "FABRIC_E_ACQUIRE_FILE_LOCK_FAILED"; return;
            case (int)FABRIC_E_CONNECTION_DENIED:  w << "FABRIC_E_CONNECTION_DENIED"; return;
            case (int)FABRIC_E_SERVER_AUTHENTICATION_FAILED: w << "FABRIC_E_SERVER_AUTHENTICATION_FAILED"; return;
            case (int)FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED: w << "FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED"; return;
            case (int)FABRIC_E_TRANSACTION_ABORTED: w << "FABRIC_E_TRANSACTION_ABORTED"; return;

            // case GatewayUnreachable: w << "GatewayUnreachable";
            case (int)FABRIC_E_GATEWAY_NOT_REACHABLE: w << "FABRIC_E_GATEWAY_NOT_REACHABLE"; return;

            case (int)FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED: w << "FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED"; return;

            case (int)FABRIC_E_CANNOT_CONNECT: w << "FABRIC_E_CANNOT_CONNECT"; return;

            case(int)FABRIC_E_MESSAGE_TOO_LARGE: w << "FABRIC_E_MESSAGE_TOO_LARGE"; return;

            case(int)FABRIC_E_ENDPOINT_NOT_FOUND: w << "FABRIC_E_ENDPOINT_NOT_FOUND"; return;

            case(int)FABRIC_E_DELETE_BACKUP_FILE_FAILED: w << "FABRIC_E_DELETE_BACKUP_FILE_FAILED"; return;

            case(int)FABRIC_E_INVALID_TEST_COMMAND_STATE: w << "FABRIC_E_INVALID_TEST_COMMAND_STATE"; return;

            case(int)FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS: w << "FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS"; return;

            case(int)FABRIC_E_CM_OPERATION_FAILED: w << "FABRIC_E_CM_OPERATION_FAILED"; return;

            case (int)FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR: w << "FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR"; return;

            case (int)FABRIC_E_CERTIFICATE_NOT_FOUND: w << "FABRIC_E_CERTIFICATE_NOT_FOUND"; return;

            case (int)FABRIC_E_CHAOS_ALREADY_RUNNING: w << "FABRIC_E_CHAOS_ALREADY_RUNNING"; return;

            case (int)FABRIC_E_FABRIC_DATA_ROOT_NOT_FOUND: w << "FABRIC_E_FABRIC_DATA_ROOT_NOT_FOUND"; return;

            case (int)FABRIC_E_STOP_IN_PROGRESS: w << "FABRIC_E_STOP_IN_PROGRESS"; return;

            case (int)FABRIC_E_ALREADY_STOPPED: w << "FABRIC_E_ALREADY_STOPPED"; return;

            case (int)FABRIC_E_NODE_IS_DOWN: w << "FABRIC_E_NODE_IS_DOWN"; return;

            case (int)FABRIC_E_NODE_TRANSITION_IN_PROGRESS: w << "FABRIC_E_NODE_TRANSITION_IN_PROGRESS"; return;

            case (int)FABRIC_E_INVALID_INSTANCE_ID: w << "FABRIC_E_INVALID_INSTANCE_ID"; return;

            case (int)FABRIC_E_INVALID_DURATION: w << "FABRIC_E_INVALID_DURATION"; return;

            case (int)FABRIC_E_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE: w << "FABRIC_E_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE"; return;

            case (int)FABRIC_E_UPLOAD_SESSION_ID_CONFLICT: w << "FABRIC_E_UPLOAD_SESSION_ID_CONFLICT"; return;

            case (int)FABRIC_E_CONFIG_UPGRADE_FAILED: w << "FABRIC_E_CONFIG_UPGRADE_FAILED"; return;

            case (int)FABRIC_E_INVALID_PARTITION_SELECTOR: w << "FABRIC_E_INVALID_PARTITION_SELECTOR"; return;

            case (int)FABRIC_E_INVALID_REPLICA_SELECTOR: w << "FABRIC_E_INVALID_REPLICA_SELECTOR"; return;

            case (int)FABRIC_E_DNS_SERVICE_NOT_FOUND: w << "FABRIC_E_DNS_SERVICE_NOT_FOUND"; return;

            case (int)FABRIC_E_INVALID_DNS_NAME: w << "FABRIC_E_INVALID_DNS_NAME"; return;

            case (int)FABRIC_E_DNS_NAME_IN_USE: w << "FABRIC_E_DNS_NAME_IN_USE"; return;

            case (int)FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS: w << "FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND: w << "FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND"; return;

            case (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING: w << "FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING"; return;

            case (int)FABRIC_E_INVALID_FOR_STATEFUL_SERVICES: w << "FABRIC_E_INVALID_FOR_STATEFUL_SERVICES"; return;

            case (int)FABRIC_E_INVALID_FOR_STATELESS_SERVICES: w << "FABRIC_E_INVALID_FOR_STATELESS_SERVICES"; return;

            case (int)FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES: w << "FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES"; return;

            case (int)FABRIC_E_INVALID_UPLOAD_SESSION_ID: w << "FABRIC_E_INVALID_UPLOAD_SESSION_ID"; return;

            case (int)FABRIC_E_BACKUP_NOT_ENABLED: w << "FABRIC_E_BACKUP_NOT_ENABLED"; return;

            case (int)FABRIC_E_BACKUP_IS_ENABLED : w << "FABRIC_E_BACKUP_IS_ENABLED"; return;

            case (int)FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST : w << "FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST"; return;

            case (int)FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS : w << "FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_RESTORE_IN_PROGRESS : w << "FABRIC_E_RESTORE_IN_PROGRESS"; return;

            case (int)FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH : w << "FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH"; return;

            case (int)FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED : w << "FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED"; return;
            
            case (int)FABRIC_E_CONTAINER_NOT_FOUND: w << "FABRIC_E_CONTAINER_NOT_FOUND"; return;

            case (int)FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC: w << "FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC"; return;

            case (int)FABRIC_E_SECRET_INVALID: w << "FABRIC_E_SECRET_INVALID"; return;

            case (int)FABRIC_E_SECRET_TYPE_CANNOT_BE_CHANGED: w << "FABRIC_E_SECRET_TYPE_CANNOT_BE_CHANGED"; return;

            case (int)FABRIC_E_SECRET_VERSION_ALREADY_EXISTS: w << "FABRIC_E_SECRET_VERSION_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_BACKUPCOPIER_UNEXPECTED_ERROR: w << "FABRIC_E_BACKUPCOPIER_UNEXPECTED_ERROR"; return;

            case (int)FABRIC_E_BACKUPCOPIER_TIMEOUT: w << "FABRIC_E_BACKUPCOPIER_TIMEOUT"; return;

            case (int)FABRIC_E_BACKUPCOPIER_ACCESS_DENIED: w << "FABRIC_E_BACKUPCOPIER_ACCESS_DENIED"; return;

            case (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS: w << "FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND: w << "FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND"; return;

            case (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS: w << "FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS"; return;

            case (int)FABRIC_E_VOLUME_ALREADY_EXISTS: w << "FABRIC_E_VOLUME_ALREADY_EXISTS"; return;

            case (int)FABRIC_E_VOLUME_NOT_FOUND: w << "FABRIC_E_VOLUME_NOT_FOUND"; return;

            case (int)FABRIC_E_INVALID_SERVICE_SCALING_POLICY: w << "FABRIC_E_INVALID_SERVICE_SCALING_POLICY"; return;
            
            case (int)FABRIC_E_NETWORK_NOT_FOUND: w << "FABRIC_E_NETWORK_NOT_FOUND"; return;

            case (int)FABRIC_E_NETWORK_IN_USE: w << "FABRIC_E_NETWORK_IN_USE"; return;

            case (int)FABRIC_E_ENDPOINT_NOT_REFERENCED: w << "FABRIC_E_ENDPOINT_NOT_REFERENCED"; return;

            case (int)FABRIC_E_DATABASE_MIGRATION_IN_PROGRESS: w << "FABRIC_E_DATABASE_MIGRATION_IN_PROGRESS"; return;

            case (int)FABRIC_E_OPERATION_NOT_SUPPORTED: w << "FABRIC_E_OPERATION_NOT_SUPPORTED"; return;

            // *** internal error codes
            case FSSPrimaryInDatalossRecovery: w << "FSSPrimaryInDatalossRecovery"; return;
            case BackupCopierAborted: w << "BackupCopierAborted"; return;
            case BackupCopierDisabled: w << "BackupCopierDisabled"; return;
            case BackupCopierRetryableError: w << "BackupCopierRetryableError"; return;

            case OperationsPending: w << "OperationsPending"; return;
            case MaxResultsReached: w << "MaxResultsReached"; return;

            case JobQueueFull: w << "JobQueueFull"; return;

            case RoutingError: w << "RoutingError"; return;
            case P2PError: w << "P2PError"; return;
            case FMLocationNotKnown: w << "FMLocationNotKnown"; return;

            case ImageBuilderAborted: w << "ImageBuilderAborted"; return;
            case ImageBuilderDisabled: w << "ImageBuilderDisabled"; return;

            case MaxFileStreamFullCopyWaiters: w << "MaxFileStreamFullCopyWaiters"; return;

            case DatabaseFilesCorrupted: w << "DatabaseFilesCorrupted"; return;

            case ServiceNotificationFilterNotFound: w << "ServiceNotificationFilterNotFound"; return;
            case ServiceNotificationFilterAlreadyExists: w << "ServiceNotificationFilterAlreadyExists"; return;

            case DuplicateMessage: w << "DuplicateMessage"; return;

            case NodeDeactivationInProgress: w << "NodeDeactivationInProgress"; return;
            case InvestigationRequired: w << "InvestigationRequired"; return;

            case HealthCheckPending: w << "HealthCheckPending"; return;

            case FabricDataRootNotFoundDeprecatedDoNotUse: w << "FabricDataRootNotFoundDeprecatedDoNotUse"; return;
            case FabricLogRootNotFound: w << "FabricLogRootNotFound"; return;
            case FabricBinRootNotFound: w << "FabricBinRootNotFound"; return;
            case FabricCodePathNotFound: w << "FabricCodePathNotFound"; return;

            case StoreOperationCanceled: w << "StoreOperationCanceled"; return;

            case InfrastructureTaskInProgress: w << "InfrastructureTaskInProgress"; return;

            case SequenceStreamRangeGapError: w << "SequenceStreamRangeGapError"; return;

            case StaleInfrastructureTask: w << "StaleInfrastructureTask"; return;
            case InfrastructureTaskNotFound: w << "InfrastructureTaskNotFound"; return;

            case SystemServiceNotFound: w << "SystemServiceNotFound"; return;

            case HealthEntityDeleted: w << "HealthEntityDeleted"; return;

            case IncompatibleVersion: w << "IncompatibleVersion"; return;

            case SerializationError: w << "SerializationError"; return;
            case OwnerExists: w << "OwnerExists"; return;
            case NotOwner: w << "NotOwner"; return;
            case VoteStoreAccessError: w << "VoteStoreAccessError"; return;

            case UnspecifiedError: w << "UnspecifiedError"; return;
            case NotFound: w << "NotFound"; return;
            case AlreadyExists: w << "AlreadyExists"; return;
            case AddressAlreadyInUse: w << "AddressAlreadyInUse"; return;
            case StaleRequest: w << "StaleRequest"; return;

            case TransportSendQueueFull: w << "TransportSendQueueFull"; return;
            case CannotConnectToAnonymousTarget: w << "CannotConnectToAnonymousTarget"; return;
            case TooManyIpcDisconnect: w << "TooManyIpcDisconnect"; return;

            case RegisterWithLeaseDriverFailed: w << "RegisterWithLeaseDriverFailed"; return;
            case LeaseFailed: w << "LeaseFailed"; return;

            case NeighborhoodLost: w << "NeighborhoodLost"; return;
            case GlobalLeaseLost: w << "GlobalLeaseLost"; return;
            case TokenAcquireTimeout: w << "TokenAcquireTimeout"; return;

            case StoreTransactionNotActiveDeprecated: w << "StoreTransactionNotActiveDeprecated"; return;
            case StoreUnexpectedError: w << "StoreUnexpectedError"; return;
            case StoreNeedsDefragment: w << "StoreNeedsDefragment"; return;
            case StoreInUse: w << "StoreInUse"; return;

            case UpdatePending: w << "UpdatePending"; return;

            case SqlStoreUnableToInitializeCommands: w << "SqlStoreUnableToInitializeCommands"; return;
            case SqlStoreRollbackTransFailed: w << "SqlStoreRollbackTransFailed"; return;
            case SqlStoreCommitTransFailed: w << "SqlStoreCommitTransFailed"; return;
            case SqlTransactionInactive: w << "SqlTransactionInactive"; return;
            case SqlStoreTransactionAlreadyCommitted: w << "SqlStoreTransactionAlreadyCommitted"; return;
            case SqlStoreTableNotFound: w << "SqlStoreTableNotFound"; return;

            case NodeIsStopped: w << "NodeIsStopped"; return;
            case P2PNodeDoesNotMatchFault: w << "P2PNodeDoesNotMatchFault"; return;
            case RoutingNodeDoesNotMatchFault: w << "RoutingNodeDoesNotMatchFault"; return;
            case NodeIsNotRoutingFault: w << "NodeIsNotRoutingFault"; return;
            case MaxRetriesReachedFault: w << "MaxRetriesReachedFault"; return;
            case BroadcastFailed: w << "BroadcastFailed"; return;
            case MessageHandlerDoesNotExistFault: w << "MessageHandlerDoesNotExistFault"; return;

            case NameUndergoingRepair: w << "NameUndergoingRepair"; return;
            case RepairContradictedOperation: w << "RepairContradictedOperation"; return;

            case ConsistencyUnitNotFound: w << "ConsistencyUnitNotFound"; return;
            case FMFailoverUnitNotFound: w << "FMFailoverUnitNotFound"; return;
            case FMStoreNotUsable: w << "FMStoreNotUsable"; return;
            case FMNotReadyForUse: w << "FMNotReadyForUse"; return;
            case FMStoreKeyNotFound: w << "FMStoreKeyNotFound"; return;
            case FMStoreUpdateFailed: w << "FMStoreUpdateFailed"; return;
            case FMServiceAlreadyExists: w << "FMServiceAlreadyExists"; return;
            case FMServiceDeleteInProgress: w << "FMServiceDeleteInProgress"; return;
            case FMServiceDoesNotExist: w << "FMServiceDoesNotExist"; return;
            case FMApplicationUpgradeInProgress: w << "FMApplicationUpgradeInProgress"; return;
            case FMFailoverUnitAlreadyExists: w << "FMFailoverUnitAlreadyExists"; return;
            case FMInvalidRolloutVersion: w << "FMInvalidRolloutVersion"; return;

            case CMRequestAborted: w << "CMRequestAborted"; return;
            case CMRequestAlreadyProcessing: w << "CMRequestAlreadyProcessing"; return;
            case CMBusy: w << "CMBusy"; return;
            case CMImageBuilderRetryableError: w << "CMImageBuilderRetryableError"; return;

            case RAServiceTypeNotRegistered: w << "RAServiceTypeNotRegistered"; return;
            case RANotReadyForUse: w << "RANotReadyForUse"; return;
            case RACouldNotCreateStoreDirectory: w << "RACouldNotCreateStoreDirectory"; return;
            case RAFailoverUnitNotFound: w << "RAFailoverUnitNotFound"; return;
            case RAStoreNotUsable: w << "RAStoreNotUsable"; return;
            case RAStoreKeyNotFound: w << "RAStoreKeyNotFound"; return;
            case RAProxyCouldNotCreateServiceObject: w << "RAProxyCouldNotCreateServiceObject"; return;
            case RAProxyCouldNotOpenStatelessService: w << "RAProxyCouldNotOpenStatelessService"; return;
            case RAProxyCouldNotCloseStatelessService: w << "RAProxyCouldNotCloseStatelessService"; return;
            case RAProxyCouldNotOpenStatefulService: w << "RAProxyCouldNotOpenStatefulService"; return;
            case RAProxyCouldNotCloseStatefulService: w << "RAProxyCouldNotCloseStatefulService"; return;
            case RAProxyCouldNotChangeRoleForStatefulService: w << "RAProxyCouldNotChangeRoleForStatefulService"; return;
            case RAProxyDemoteCompleted: w << "RAProxyDemoteCompleted"; return;
            case RAProxyBuildIdleReplicaInProgress: w << "RAProxyBuildIdleReplicaInProgress"; return;
            case RAProxyOperationIncompatibleWithCurrentFupState: w << "RAProxyOperationIncompatibleWithCurrentFupState"; return;
            case RAProxyUpdateReplicatorConfigurationPending: w << "RAProxyUpdateReplicatorConfigurationPending"; return;
            case RAProxyStateChangedOnDataLoss: w << "RAProxyStateChangedOnDataLoss"; return;
            case REDuplicateOperation: w << "REDuplicateOperation"; return;

            // FileLock
            case SharingAccessLockViolation: w << "SharingAccessLockViolation"; return;

                //
                // Hosting
                //
            case HostingServiceTypeRegistrationVersionMismatch: w << "HostingServiceTypeRegistrationVersionMismatch"; return;
            case HostingServicePackageVersionMismatch: w << "HostingServicePackageVersionMismatch"; return;
            case HostingApplicationVersionMismatch: w << "HostingApplicationVersionMismatch"; return;
            case HostingApplicationHostAlreadyRegistered: w << "HostingApplicationHostAlreadyRegistered"; return;
            case HostingApplicationHostNotRegistered: w << "HostingApplicationHostNotRegistered"; return;
            case HostingFabricRuntimeAlreadyRegistered: w << "HostingFabricRuntimeAlreadyRegistered"; return;
            case HostingFabricRuntimeNotRegistered: w << "HostingFabricRuntimeNotRegistered"; return;
            case HostingServiceTypeAlreadyRegisteredToSameRuntime: w << "HostingServiceTypeAlreadyRegisteredToSameRuntime"; return;
            case HostingServiceTypeNotOwned: w << "HostingServiceTypeNotOwned"; return;
            case HostingServiceTypeDisabled: w << "HostingServiceTypeDisabled"; return;
            case HostingDeploymentInProgress: w << "HostingDeploymentInProgress"; return;
            case HostingActivationInProgress: w << "HostingActivationInProgress"; return;
            case HostingCodePackageNotHosted: w << "HostingCodePackageNotHosted"; return;
            case HostingCodePackageAlreadyHosted: w << "HostingCodePackageAlreadyHosted"; return;
            case HostingDllHostNotFound: w << "HostingDllHostNotFound"; return;
            case HostingTypeHostNotFound: w << "HostingTypeHostNotFound"; return;
            case EndpointProviderPortRangeExhausted : w << "EndpointProviderPortRangeExhausted"; return;

            case EndpointProviderNotEnabled : w << "EndpointProviderNotEnabled"; return;
            case ApplicationManagerApplicationAlreadyExists: w << "ApplicationManagerApplicationAlreadyExists"; return;
            case ApplicationManagerApplicationNotFound: w << "ApplicationManagerApplicationNotFound"; return;
            case ApplicationPrincipalDoesNotExist: w << "ApplicationPrincipalDoesNotExist"; return;
            case HostingActivationEntityNotInUse: w << "HostingActivationEntityNotInUse"; return;
            case HostingEntityAborted: w << "HostingEntityAborted"; return;
            case ApplicationInstanceDeleted: w << "ApplicationInstanceDeleted"; return;
            case ReliableSessionConflictingSessionAborted: w << "ReliableSessionConflictingSessionAborted"; return;

            case ImageStoreInvalidStoreUri: w << "ImageStoreInvalidStoreUri"; return;
            case ImageStoreUnableToPerformAzureOperation: w << "ImageStoreUnableToPerformAzureOperation"; return;
            case AbandonedFileWriteLockFound: w << "AbandonedFileWriteLockFound"; return;
            case HostingServiceTypeValidationInProgress: w << "HostingServiceTypeValidationInProgress"; return;
            case ContainerFeatureNotEnabled: w << "ContainerFeatureNotEnabled"; return;
            case ContainerFailedToStart: w << "ContainerFailedToStart"; return;
            case ContainerCreationFailed: w << "ContainerCreationFailed"; return;
            case ContainerFailedToCreateDnsChain: w << "ContainerFailedToCreateDnsChain"; return;
            case ApplicationPrincipalAbortableError: w << "ApplicationPrincipalAbortableError"; return;
            case ApplicationHostCrash: w << "ApplicationHostCrash"; return;
            case ApplicationDeploymentInProgress: w << "ApplicationDeploymentInProgress"; return;
            case UpdateContextFailed: w << "UpdateContextFailed"; return;

                // FileStoreService
            case StagingFileNotFound: w << "StagingFileNotFound"; return;
            case FileUpdateInProgress: w << "FileUpdateInProgress"; return;
            case FileAlreadyExists: w << "FileAlreadyExists"; return;
            case FileStoreServiceNotReady: w << "FileStoreServiceNotReady"; return;
            case FileStoreServiceReplicationProcessingError: w << "FileStoreServiceReplicationProcessingError"; return;

                //FabricTest
            case FabricTestServiceNotOpen: w << "FabricTestServiceNotOpen"; return;
            case FabricTestIncorrectServiceLocation: w << "FabricTestIncorrectServiceLocation"; return;
            case FabricTestStatusNotGranted: w << "FabricTestStatusNotGranted"; return;
            case FabricTestVersionDoesNotMatch: w << "FabricTestVersionDoesNotMatch"; return;
            case FabricTestKeyDoesNotExist: w << "FabricTestKeyDoesNotExist"; return;
            case FabricTestReplicationFailed: w << "FabricTestReplicationFailed"; return;

            case XmlInvalidContent: w << "XmlInvalidContent"; return;
            case XmlUnexpectedEndOfFile: w << "XmlUnexpectedEndOfFile"; return;
            case InvalidServiceTypeV1: w << "InvalidServiceTypeV1"; return;
            case OperationStreamFaulted: w << "OperationStreamFaulted"; return;
            case StoreFatalError: w << "StoreFatalError"; return;

            case SecuritySessionExpiredByRemoteEnd: w << "SecuritySessionExpiredByRemoteEnd"; return;
            case ConnectionClosedByRemoteEnd: w << "FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END"; return;
            case MessageExpired: w << "MessageExpired"; return;
            case SecurityNegotiationTimeout: w << "SecurityNegotiationTimeout"; return;
            case SecuritySessionExpired: w << "SecuritySessionExpired"; return;

            case ConnectionInstanceObsolete: w << "ConnectionInstanceObsolete"; return;
            case ConnectionConfirmWaitExpired: w << "ConnectionConfirmWaitExpired"; return;
            case ConnectionIdleTimeout: w << "ConnectionIdleTimeout"; return;
            case CannotConnect: w << "CannotConnect"; return;
            case CertificateNotMatched: w << "CertificateNotMatched"; return;
            case CertificateNotFound_DummyPlaceHolder: w << "CertificateNotFound_DummyPlaceHolder"; return;
            case CredentialAlreadyLoaded: w << "CredentialAlreadyLoaded"; return;

            case StaleServicePackageInstance: w << "StaleServicePackageInstance"; return;

            case IncomingConnectionThrottled: w << "IncomingConnectionThrottled"; return;

            case IPAddressProviderAddressRangeExhausted: w << "IPAddressProviderAddressRangeExhausted"; return;

            case NatIpAddressProviderAddressRangeExhausted: w << "NatIpAddressProviderAddressRangeExhausted"; return;

            case ServiceHostTerminationInProgress: w << "ServiceHostTerminationInProgress"; return;

            case OverlayNetworkResourceProviderAddressRangeExhausted: w << "OverlayNetworkResourceProviderAddressRangeExhausted"; return;

            case FabricRemoveConfigurationValueNotFound: w << "FabricRemoveConfigurationValueNotFound"; return;

            case ReplicatorInternalError: w << "ReplicatorInternalError"; return;

            case RebootRequired: w << "RebootRequired"; return;

            case InconsistentInMemoryState: w << "InconsistentInMemoryState"; return;

            case NamingServiceTooBusy: w << "NamingServiceTooBusy"; return;

            case DnsServerIPAddressNotFound: w << "DnsServerIPAddressNotFound"; return;

            case IsolatedNetworkInterfaceNameNotFound: w << "IsolatedNetworkInterfaceNameNotFound"; return;

            case LocalResourceManagerCPUCapacityMismatch: w << "LocalResourceManagerCPUCapacityMismatch"; return;

            case LocalResourceManagerMemoryCapacityMismatch: w << "LocalResourceManagerMemoryCapacityMismatch"; return;

            case NotEnoughCPUForServicePackage: w << "NotEnoughCPUForServicePackage"; return;

            case NotEnoughMemoryForServicePackage: w << "NotEnoughMemoryForServicePackage"; return;

            case ServicePackageAlreadyRegisteredWithLRM: w << L"ServicePackageAlreadyRegisteredWithLRM"; return;

            case FabricHostServicePathNotFound: w << L"FabricHostServicePathNotFound"; return;

            case UpdaterServicePathNotFound: w << L"UpdaterServicePathNotFound"; return;
            }

            w.Write("0x{0:x}", static_cast<uint>(e));
        }

        BEGIN_ENUM_STRUCTURED_TRACE( ErrorCodeValue )

            // common HRESULTs (only those used by Fabric)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, S_OK)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_NOTIMPL)
            //ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_NOINTERFACE)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_POINTER)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_ABORT)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_FAIL)
            //ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_UNEXPECTED)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_ACCESSDENIED)
            //ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_HANDLE)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_OUTOFMEMORY)
            ADD_CASTED_ENUM_MAP_VALUE( ErrorCodeValue, E_INVALIDARG)

            // public Fabric HRESULTs
            ADD_CASTED_ENUM_MAP_VALUE_RANGE( ErrorCodeValue, FABRIC_E_FIRST_RESERVED_HRESULT, FABRIC_E_LAST_USED_HRESULT )

            // internal error codes
            ADD_CASTED_ENUM_MAP_VALUE_RANGE( ErrorCodeValue, (FIRST_INTERNAL_ERROR_CODE_MINUS_ONE + 1), LAST_INTERNAL_ERROR_CODE )

            END_ENUM_STRUCTURED_TRACE( ErrorCodeValue )

    } // end ErrorCodeValue namespace
}