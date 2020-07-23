// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;


ErrorBody::ErrorBody(HttpGateway::ErrorCodeMessageBody errorCodeMessageBody)
: errorCodeMessageBody_(errorCodeMessageBody)
{
}

std::wstring ErrorBody::GetErrorCode()
{
    return errorCodeMessageBody_.GetErrorCode();
}

std::wstring ErrorBody::GetErrorMessage()
{
    return errorCodeMessageBody_.GetErrorMessage();
}

ErrorBody ErrorBody::CreateErrorBodyFromErrorCode(Common::ErrorCode errorCode)
{
    ErrorCodeMessageBody errorCodeMessageBody;
    errorCodeMessageBody.SetErrorCode(FromErrorCode(errorCode));
    if (!errorCode.Message.empty())
    {
        errorCodeMessageBody.SetErrorMessage(errorCode.Message);
    }
    else
    {
        errorCodeMessageBody.SetErrorMessage(FromErrorMessage(errorCode));
    }

    return ErrorBody(errorCodeMessageBody);
}

std::wstring ErrorBody::FromErrorCode(Common::ErrorCode errorCode)
{
    wstring errorCodeString;
    StringWriter w(errorCodeString);
    w.Write("{0}", errorCode);

    return errorCodeString;
}

std::wstring ErrorBody::FromErrorMessage(Common::ErrorCode errorCode)
{
    std::wstring errorMessage;
    
    switch (errorCode.ToHResult())
    {
    case STATUS_SUCCESS:
        Common::Assert::CodingError("Http gateway getting an error message of a success code");
        break;
    case (int) FABRIC_E_INVALID_PARTITION_KEY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_KEY);
        break;
    case (int) FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED);
        break;
    case (int) FABRIC_E_NAME_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NAME_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_SERVICE_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_APPLICATION_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_NAME_DOES_NOT_EXIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NAME_DOES_NOT_EXIST);
        break;
    case (int) FABRIC_E_PROPERTY_DOES_NOT_EXIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PROPERTY_DOES_NOT_EXIST);
        break;
    case (int) FABRIC_E_SERVICE_DOES_NOT_EXIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_DOES_NOT_EXIST);
        break;
    case (int) FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST);
        break;
    case (int) FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS);
        break;
    case (int) FABRIC_E_APPLICATION_TYPE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_NOT_FOUND);
        break;
    case (int) FABRIC_E_APPLICATION_TYPE_IN_USE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_TYPE_IN_USE);
        break;
    case (int) FABRIC_E_APPLICATION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_NOT_FOUND);
        break;
    case (int) FABRIC_E_SERVICE_TYPE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_NOT_FOUND);
        break;
    case (int) FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND);
        break;
    case (int) FABRIC_E_SERVICE_MANIFEST_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_MANIFEST_NOT_FOUND);
        break;
    case (int) FABRIC_E_NOT_PRIMARY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NOT_PRIMARY);
        break;
    case (int) FABRIC_E_NO_WRITE_QUORUM:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NO_WRITE_QUORUM);
        break;
    case (int) FABRIC_E_RECONFIGURATION_PENDING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RECONFIGURATION_PENDING);
        break;
    case (int) FABRIC_E_REPLICATION_QUEUE_FULL:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPLICATION_QUEUE_FULL);
        break;
    case (int) FABRIC_E_REPLICATION_OPERATION_TOO_LARGE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPLICATION_OPERATION_TOO_LARGE);
        break;
    case (int) FABRIC_E_INVALID_ATOMIC_GROUP:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ATOMIC_GROUP);
        break;
    case (int) FABRIC_E_SERVICE_OFFLINE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_OFFLINE);
        break;
    case (int) FABRIC_E_SERVICE_METADATA_MISMATCH:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_METADATA_MISMATCH);
        break;
    case (int) FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED);
        break;
    case (int) FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS);
        break;
    case (int) FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS);
        break;
    case (int) FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED);
        break;
    case (int) FABRIC_E_FABRIC_VERSION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_NOT_FOUND);
        break;
    case (int) FABRIC_E_FABRIC_VERSION_IN_USE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_IN_USE);
        break;
    case (int) FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION);
        break;
    case (int) FABRIC_E_FABRIC_NOT_UPGRADING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_NOT_UPGRADING);
        break;
    case (int) FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS);
        break;
    case (int) FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR);
        break;
    case (int) FABRIC_E_HEALTH_MAX_REPORTS_REACHED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_MAX_REPORTS_REACHED);
        break;
    case (int) FABRIC_E_HEALTH_STALE_REPORT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_STALE_REPORT);
        break;
    case (int) FABRIC_E_HEALTH_ENTITY_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_HEALTH_ENTITY_NOT_FOUND);
        break;
    case (int) FABRIC_E_APPLICATION_NOT_UPGRADING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_NOT_UPGRADING);
        break;
    case (int) FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION);
        break;
    case (int) FABRIC_E_NODE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NODE_NOT_FOUND);
        break;
    case (int) FABRIC_E_NODE_IS_UP:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NODE_IS_UP);
        break;
    case (int) FABRIC_E_WRITE_CONFLICT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_WRITE_CONFLICT);
        break;
    case (int) FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR);
        break;
    case (int) FABRIC_E_SERVICE_TYPE_MISMATCH:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_MISMATCH);
        break;
    case (int) FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED);
        break;
    case (int) FABRIC_E_SERVICE_TYPE_NOT_REGISTERED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TYPE_NOT_REGISTERED);
        break;
    case (int) FABRIC_E_CODE_PACKAGE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CODE_PACKAGE_NOT_FOUND);
        break;
    case (int) FABRIC_E_DATA_PACKAGE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DATA_PACKAGE_NOT_FOUND);
        break;
    case (int) FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND);
        break;
    case (int) FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND);
        break;
    case (int) FABRIC_E_NAME_NOT_EMPTY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NAME_NOT_EMPTY);
        break;
    case (int) FABRIC_E_INVALID_CONFIGURATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CONFIGURATION);
        break;
    case (int) FABRIC_E_INVALID_ADDRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ADDRESS);
        break;
    case (int) FABRIC_E_INVALID_NAME_URI:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_NAME_URI);
        break;
    case (int) FABRIC_E_VALUE_TOO_LARGE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_VALUE_TOO_LARGE);
        break;
    case (int) FABRIC_E_VALUE_EMPTY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_VALUE_EMPTY);
        break;
    case (int) FABRIC_E_PROPERTY_CHECK_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PROPERTY_CHECK_FAILED);
        break;
    case (int) FABRIC_E_NOT_READY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NOT_READY);
        break;
    case (int) FABRIC_E_INVALID_CREDENTIAL_TYPE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CREDENTIAL_TYPE);
        break;
    case (int) FABRIC_E_INVALID_X509_FIND_TYPE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_FIND_TYPE);
        break;
    case (int) FABRIC_E_INVALID_X509_STORE_LOCATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE_LOCATION);
        break;
    case (int) FABRIC_E_INVALID_X509_STORE_NAME:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE_NAME);
        break;
    case (int) FABRIC_E_INVALID_X509_THUMBPRINT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_THUMBPRINT);
        break;
    case (int) FABRIC_E_INVALID_PROTECTION_LEVEL:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PROTECTION_LEVEL);
        break;
    case (int) FABRIC_E_INVALID_X509_STORE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_X509_STORE);
        break;
    case (int) FABRIC_E_INVALID_SUBJECT_NAME:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_SUBJECT_NAME);
        break;
    case (int) FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST);
        break;
    case (int) FABRIC_E_INVALID_CREDENTIALS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_CREDENTIALS);
        break;
    case (int) FABRIC_E_DECRYPTION_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DECRYPTION_FAILED);
        break;
    case (int) FABRIC_E_ENCRYPTION_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_ENCRYPTION_FAILED);
        break;
    case (int) FABRIC_E_SERVICE_TOO_BUSY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVICE_TOO_BUSY);
        break;
    case (int) FABRIC_E_COMMUNICATION_ERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_COMMUNICATION_ERROR);
        break;
    case (int) FABRIC_E_GATEWAY_NOT_REACHABLE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_GATEWAY_NOT_REACHABLE);
        break;
    case (int) FABRIC_E_OBJECT_CLOSED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_OBJECT_CLOSED);
        break;
    case (int) FABRIC_E_OPERATION_NOT_COMPLETE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_OPERATION_NOT_COMPLETE);
        break;
    case (int) FABRIC_E_ENUMERATION_COMPLETED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_ENUMERATION_COMPLETED);
        break;
    case (int) FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND);
        break;
    case (int) FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND);
        break;
    case (int) FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR);
        break;
    case (int) FABRIC_E_PARTITION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PARTITION_NOT_FOUND);
        break;
    case (int) FABRIC_E_REPLICA_DOES_NOT_EXIST:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPLICA_DOES_NOT_EXIST);
        break;
    case (int) FABRIC_E_PROCESS_DEACTIVATED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PROCESS_DEACTIVATED);
        break;
    case (int) FABRIC_E_PROCESS_ABORTED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PROCESS_ABORTED);
        break;
    case (int) FABRIC_E_KEY_TOO_LARGE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_KEY_TOO_LARGE);
        break;
    case (int) FABRIC_E_KEY_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_KEY_NOT_FOUND);
        break;
    case (int) FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED);
        break;
    case (int) FABRIC_E_TRANSACTION_NOT_ACTIVE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_NOT_ACTIVE);
        break;
    case (int) FABRIC_E_TRANSACTION_TOO_LARGE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_TOO_LARGE);
        break;
    case (int) FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED);
        break;
    case (int) FABRIC_E_TRANSACTION_ABORTED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_TRANSACTION_ABORTED);
        break;
    case E_INVALIDARG:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_INVALIDARG);
        break;
    case E_POINTER:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_POINTER);
        break;
    case E_ABORT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_ABORT);
        break;
    case E_ACCESSDENIED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_ACCESSDENIED);
        break;
    case E_NOINTERFACE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_NOINTERFACE);
        break;
    case E_NOTIMPL:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_NOTIMPL);
        break;
    case E_OUTOFMEMORY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_OUTOFMEMORY);
        break;
    case ERROR_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_E_NOT_FOUND);
        break;
    case (int) FABRIC_E_INVALID_OPERATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_OPERATION);
        break;
    case (int) FABRIC_E_TIMEOUT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_TIMEOUT);
        break;
    case (int) FABRIC_E_DIRECTORY_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DIRECTORY_NOT_FOUND);
        break;
    case (int) FABRIC_E_INVALID_DIRECTORY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_DIRECTORY);
        break;
    case (int) FABRIC_E_PATH_TOO_LONG:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PATH_TOO_LONG);
        break;
    case (int) FABRIC_E_FILE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FILE_NOT_FOUND);
        break;
    case (int) FABRIC_E_IMAGESTORE_IOERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGESTORE_IOERROR);
        break;
    case (int) FABRIC_E_ACQUIRE_FILE_LOCK_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_ACQUIRE_FILE_LOCK_FAILED);
        break;
    case (int) FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CORRUPTED_IMAGE_STORE_OBJECT_FOUND);
        break;
    case (int) FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);
        break;
    case (int) FABRIC_E_IMAGEBUILDER_TIMEOUT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_TIMEOUT);
        break;
    case (int) FABRIC_E_IMAGEBUILDER_ACCESS_DENIED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_ACCESS_DENIED);
        break;
    case (int) FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_IMAGEBUILDER_INVALID_MSI_FILE);
        break;
    case (int) FABRIC_E_INVALID_SERVICE_TYPE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_SERVICE_TYPE);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_TRANSPORT_STARTUP_FAILURE);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_CANNOT_CONNECT);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_EXISTS);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_REJECTED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_REJECTED);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_NOT_FOUND);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_QUEUE_EMPTY);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_QUOTA_EXCEEDED);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_SERVICE_FAULTED);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_ALREADY_LISTENING);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_FOUND);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_MANAGER_NOT_LISTENING);
        break;
    case (int) FABRIC_E_REPAIR_TASK_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPAIR_TASK_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_REPAIR_TASK_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPAIR_TASK_NOT_FOUND);
        break;
    case (int) FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_RELIABLE_SESSION_INVALID_TARGET_PARTITION);
        break;
    case (int) FABRIC_E_INSTANCE_ID_MISMATCH:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INSTANCE_ID_MISMATCH);
        break;
    case (int) FABRIC_E_NODE_HAS_NOT_STOPPED_YET:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_NODE_HAS_NOT_STOPPED_YET);
        break;
    case (int) FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY);
        break;
    case (int) FABRIC_E_CONSTRAINT_KEY_UNDEFINED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONSTRAINT_KEY_UNDEFINED);
        break;
    case (int) FABRIC_E_INVALID_PACKAGE_SHARING_POLICY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PACKAGE_SHARING_POLICY);
        break;
    case (int) FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED);
        break;
    case (int) FABRIC_E_INVALID_BACKUP_SETTING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_BACKUP_SETTING);
        break;
    case (int) FABRIC_E_MISSING_FULL_BACKUP:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_MISSING_FULL_BACKUP);
        break;
    case (int) FABRIC_E_BACKUP_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_BACKUP_IN_PROGRESS);
        break;
    case (int) FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_BACKUP_DIRECTORY_NOT_EMPTY);
        break;
    case (int) FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME);
        break;
    case (int) FABRIC_E_INVALID_REPLICA_OPERATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_OPERATION);
        break;
    case (int) FABRIC_E_INVALID_REPLICA_STATE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_STATE);
        break;
    case (int) FABRIC_E_LOADBALANCER_NOT_READY:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_LOADBALANCER_NOT_READY);
        break;
    case (int) FABRIC_E_INVALID_PARTITION_OPERATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_OPERATION);
        break;
    case (int) FABRIC_E_PRIMARY_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_PRIMARY_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_SECONDARY_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SECONDARY_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_FORCE_NOT_SUPPORTED_FOR_REPLICA_OPERATION);
        break;
    case (int) FABRIC_E_CONNECTION_DENIED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONNECTION_DENIED);
        break;
    case (int) FABRIC_E_SERVER_AUTHENTICATION_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SERVER_AUTHENTICATION_FAILED);
        break;
    case (int) FABRIC_E_CANNOT_CONNECT:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CANNOT_CONNECT);
        break;
    case (int) FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END);
        break;
    case (int) FABRIC_E_MESSAGE_TOO_LARGE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_MESSAGE_TOO_LARGE);
        break;
    case (int) FABRIC_E_CONSTRAINT_NOT_SATISFIED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONSTRAINT_NOT_SATISFIED);
        break;
    case (int) FABRIC_E_ENDPOINT_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_ENDPOINT_NOT_FOUND);
        break;
    case (int) FABRIC_E_DELETE_BACKUP_FILE_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DELETE_BACKUP_FILE_FAILED);
        break;
    case (int) FABRIC_E_INVALID_TEST_COMMAND_STATE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_TEST_COMMAND_STATE);
        break;
    case (int) FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_CM_OPERATION_FAILED:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CM_OPERATION_FAILED);
        break;
    case (int) FABRIC_E_INVALID_PARTITION_SELECTOR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_PARTITION_SELECTOR);
        break;
    case (int) FABRIC_E_INVALID_REPLICA_SELECTOR:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_REPLICA_SELECTOR);
        break;
    case (int)FABRIC_E_DNS_SERVICE_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DNS_SERVICE_NOT_FOUND);
        break;
    case (int)FABRIC_E_INVALID_DNS_NAME:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_DNS_NAME);
        break;
    case (int)FABRIC_E_DNS_NAME_IN_USE:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_DNS_NAME_IN_USE);
        break;
    case (int) FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND);
        break;
    case (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING);
        break;
    case (int) FABRIC_E_INVALID_FOR_STATEFUL_SERVICES:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_FOR_STATEFUL_SERVICES);
        break;
    case (int) FABRIC_E_INVALID_FOR_STATELESS_SERVICES:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_FOR_STATELESS_SERVICES);
        break;
    case (int) FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES);
        break;
    case (int)FABRIC_E_CONTAINER_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CONTAINER_NOT_FOUND);
        break;
    case (int) FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_NOT_FOUND);
        break;
    case (int) FABRIC_E_VOLUME_ALREADY_EXISTS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_VOLUME_ALREADY_EXISTS);
        break;
    case (int) FABRIC_E_VOLUME_NOT_FOUND:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_VOLUME_NOT_FOUND);
        break;
    case (int)FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_CENTRAL_SECRET_SERVICE_GENERIC);
        break;
    case (int)FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS:
        errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_SINGLE_INSTANCE_APPLICATION_UPGRADE_IN_PROGRESS);
        break;
    default:
        errorMessage = L"Null";
        break;
    }
    
    return errorMessage;
}
