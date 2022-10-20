// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpCommon;

#if !defined(PLATFORM_UNIX)

bool HttpUtil::HeaderStringToKnownHeaderCode(__in wstring const&headerName, __out KHttpUtils::HttpHeaderType &headerCode)
{
    if (headerName.empty()) { return false; }
    else if (headerName == HttpConstants::ContentLengthHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderContentLength); }
    else if (headerName == HttpConstants::ContentTypeHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderContentType); }
    else if (headerName == HttpConstants::WWWAuthenticateHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderWwwAuthenticate); }
    else if (headerName == HttpConstants::TransferEncodingHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderTransferEncoding); }
    else if (headerName == HttpConstants::LocationHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderLocation); }
    else if (headerName == HttpConstants::AuthorizationHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderAuthorization); }
    else if (headerName == HttpConstants::UpgradeHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderUpgrade); }
    else if (headerName == HttpConstants::HostHeader) { headerCode = static_cast<KHttpUtils::HttpHeaderType>(HttpHeaderHost); }
    else { return false; }

    return true;
}

bool HttpUtil::VerbToKHttpOpType(std::wstring const &verb, __out KHttpUtils::OperationType &opType)
{
    if (verb == *HttpConstants::HttpGetVerb) opType = KHttpUtils::OperationType::eHttpGet;
    else if (verb == *HttpConstants::HttpPostVerb) opType = KHttpUtils::OperationType::eHttpPost;
    else if (verb == *HttpConstants::HttpPutVerb) opType = KHttpUtils::OperationType::eHttpPut;
    else if (verb == *HttpConstants::HttpDeleteVerb) opType = KHttpUtils::OperationType::eHttpDelete;
    else if (verb == *HttpConstants::HttpHeadVerb) opType = KHttpUtils::OperationType::eHttpHead;
    else if (verb == *HttpConstants::HttpOptionsVerb) opType = KHttpUtils::OperationType::eHttpOptions;
    else if (verb == *HttpConstants::HttpTraceVerb) opType = KHttpUtils::OperationType::eHttpTrace;
    else if (verb == *HttpConstants::HttpConnectVerb) opType = KHttpUtils::OperationType::eHttpConnect;
    else
    {
        Assert::TestAssert("Unknown Http verb {0}", verb);
        return false;
    }

    return true;
}

std::wstring HttpUtil::WinHttpCodeToString(
    __in ULONG winhttpError)
{
    switch (winhttpError)
    {
    case ERROR_SUCCESS:
        return L"ERROR_SUCCESS";
    case ERROR_WINHTTP_OUT_OF_HANDLES:
        return L"ERROR_WINHTTP_OUT_OF_HANDLES";
    case ERROR_WINHTTP_TIMEOUT:
        return L"ERROR_WINHTTP_TIMEOUT";
    case ERROR_WINHTTP_INTERNAL_ERROR:
        return L"ERROR_WINHTTP_INTERNAL_ERROR";
    case ERROR_WINHTTP_INVALID_URL:
        return L"ERROR_WINHTTP_INVALID_URL";
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
        return L"ERROR_WINHTTP_UNRECOGNIZED_SCHEME";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        return L"ERROR_WINHTTP_NAME_NOT_RESOLVED";
    case ERROR_WINHTTP_INVALID_OPTION:
        return L"ERROR_WINHTTP_INVALID_OPTION";
    case ERROR_WINHTTP_OPTION_NOT_SETTABLE:
        return L"ERROR_WINHTTP_OPTION_NOT_SETTABLE";
    case ERROR_WINHTTP_SHUTDOWN:
        return L"ERROR_WINHTTP_SHUTDOWN";
    case ERROR_WINHTTP_LOGIN_FAILURE:
        return L"ERROR_WINHTTP_LOGIN_FAILURE";
    case ERROR_WINHTTP_OPERATION_CANCELLED:
        return L"ERROR_WINHTTP_OPERATION_CANCELLED";
    case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
        return L"ERROR_WINHTTP_INCORRECT_HANDLE_TYPE";
    case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
        return L"ERROR_WINHTTP_INCORRECT_HANDLE_STATE";
    case ERROR_WINHTTP_CANNOT_CONNECT:
        return L"ERROR_WINHTTP_CANNOT_CONNECT";
    case ERROR_WINHTTP_CONNECTION_ERROR:
        return L"ERROR_WINHTTP_CONNECTION_ERROR";
    case ERROR_WINHTTP_RESEND_REQUEST:
        return L"ERROR_WINHTTP_RESEND_REQUEST";
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
        return L"ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED";
    case ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN:
        return L"ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN";
    case ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND:
        return L"ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND";
    case ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND:
        return L"ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND";
    case ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN:
        return L"ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN";
    case ERROR_WINHTTP_HEADER_NOT_FOUND:
        return L"ERROR_WINHTTP_HEADER_NOT_FOUND";
    case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
        return L"ERROR_WINHTTP_INVALID_SERVER_RESPONSE";
    case ERROR_WINHTTP_INVALID_HEADER:
        return L"ERROR_WINHTTP_INVALID_HEADER";
    case ERROR_WINHTTP_INVALID_QUERY_REQUEST:
        return L"ERROR_WINHTTP_INVALID_QUERY_REQUEST";
    case ERROR_WINHTTP_HEADER_ALREADY_EXISTS:
        return L"ERROR_WINHTTP_HEADER_ALREADY_EXISTS";
    case ERROR_WINHTTP_REDIRECT_FAILED:
        return L"ERROR_WINHTTP_REDIRECT_FAILED";
    case ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR:
        return L"ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR";
    case ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT:
        return L"ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT";
    case ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT:
        return L"ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT";
    case ERROR_WINHTTP_UNHANDLED_SCRIPT_TYPE:
        return L"ERROR_WINHTTP_UNHANDLED_SCRIPT_TYPE";
    case ERROR_WINHTTP_SCRIPT_EXECUTION_ERROR:
        return L"ERROR_WINHTTP_SCRIPT_EXECUTION_ERROR";
    case ERROR_WINHTTP_NOT_INITIALIZED:
        return L"ERROR_WINHTTP_NOT_INITIALIZED";
    case ERROR_WINHTTP_SECURE_FAILURE:
        return L"ERROR_WINHTTP_SECURE_FAILURE";
    case ERROR_WINHTTP_SECURE_CERT_DATE_INVALID:
        return L"ERROR_WINHTTP_SECURE_CERT_DATE_INVALID";
    case ERROR_WINHTTP_SECURE_CERT_CN_INVALID:
        return L"ERROR_WINHTTP_SECURE_CERT_CN_INVALID";
    case ERROR_WINHTTP_SECURE_INVALID_CA:
        return L"ERROR_WINHTTP_SECURE_INVALID_CA";
    case ERROR_WINHTTP_SECURE_CERT_REV_FAILED:
        return L"ERROR_WINHTTP_SECURE_CERT_REV_FAILED";
    case ERROR_WINHTTP_SECURE_CHANNEL_ERROR:
        return L"ERROR_WINHTTP_SECURE_CHANNEL_ERROR";
    case ERROR_WINHTTP_SECURE_INVALID_CERT:
        return L"ERROR_WINHTTP_SECURE_INVALID_CERT";
    case ERROR_WINHTTP_SECURE_CERT_REVOKED:
        return L"ERROR_WINHTTP_SECURE_CERT_REVOKED";
    case ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE:
        return L"ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE";
    case ERROR_WINHTTP_AUTODETECTION_FAILED:
        return L"ERROR_WINHTTP_AUTODETECTION_FAILED";
    case ERROR_WINHTTP_HEADER_COUNT_EXCEEDED:
        return L"ERROR_WINHTTP_HEADER_COUNT_EXCEEDED";
    case ERROR_WINHTTP_HEADER_SIZE_OVERFLOW:
        return L"ERROR_WINHTTP_HEADER_SIZE_OVERFLOW";
    case ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW:
        return L"ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW";
    case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
        return L"ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW";
    case ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY:
        return L"ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY";
    case ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY:
        return L"ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY";
    default:
        return wformatString(L"Internal error {0}", winhttpError);
    }
}

#endif

bool HttpUtil::IsHttpUri(std::wstring const &uri)
{
    // 1. Scheme should start with http://
    // 2. Scheme is case insensitive.

    if (uri.size() <= (*HttpCommon::HttpConstants::HttpUriScheme).size())
    {
        return false;
    }

    if (Common::StringUtility::ToLowerCharW(uri[0]) == (*HttpCommon::HttpConstants::HttpUriScheme)[0] &&
        Common::StringUtility::ToLowerCharW(uri[1]) == (*HttpCommon::HttpConstants::HttpUriScheme)[1] &&
        Common::StringUtility::ToLowerCharW(uri[2]) == (*HttpCommon::HttpConstants::HttpUriScheme)[2] &&
        Common::StringUtility::ToLowerCharW(uri[3]) == (*HttpCommon::HttpConstants::HttpUriScheme)[3] &&
        Common::StringUtility::ToLowerCharW(uri[4]) == (*HttpCommon::HttpConstants::ProtocolSeparator)[0])
    {
        return true;
    }

    return false;
}

bool HttpUtil::IsHttpsUri(std::wstring const &uri)
{
    // 1. Scheme should start with https://
    // 2. Scheme is case insensitive.

    if (uri.size() <= (*HttpCommon::HttpConstants::HttpsUriScheme).size())
    {
        return false;
    }

    if (Common::StringUtility::ToLowerCharW(uri[0]) == (*HttpCommon::HttpConstants::HttpsUriScheme)[0] &&
        Common::StringUtility::ToLowerCharW(uri[1]) == (*HttpCommon::HttpConstants::HttpsUriScheme)[1] &&
        Common::StringUtility::ToLowerCharW(uri[2]) == (*HttpCommon::HttpConstants::HttpsUriScheme)[2] &&
        Common::StringUtility::ToLowerCharW(uri[3]) == (*HttpCommon::HttpConstants::HttpsUriScheme)[3] &&
        Common::StringUtility::ToLowerCharW(uri[4]) == (*HttpCommon::HttpConstants::HttpsUriScheme)[4] &&
        Common::StringUtility::ToLowerCharW(uri[5]) == (*HttpCommon::HttpConstants::ProtocolSeparator)[0])
    {
        return true;
    }

    return false;
}

void HttpUtil::ErrorCodeToHttpStatus(
    __in ErrorCode error,
    __out USHORT &httpStatus,
    __out wstring &httpStatusLine)
{
    switch (error.ToHResult())
    {
    case STATUS_SUCCESS:
    {
        httpStatus = HttpStatusCode::Ok;
        httpStatusLine = L"OK";
        break;
    }

    case E_INVALIDARG:
    {
        httpStatus = HttpStatusCode::BadRequest;
        httpStatusLine = L"Bad Request";
        break;
    }

    case E_ACCESSDENIED:
    {
        httpStatus = HttpStatusCode::Forbidden;
        httpStatusLine = L"Access Denied";
        break;
    }

    case E_NOTIMPL:
    {
        httpStatus = HttpStatusCode::NotImplemented;
        httpStatusLine = L"Not Implemented";
        break;
    }

    case (int)FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS:
    case (int)FABRIC_E_APPLICATION_ALREADY_EXISTS:
    case (int)FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION:
    case (int)FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS:
    case (int)FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS:
    case (int)FABRIC_E_SERVICE_ALREADY_EXISTS:
    case (int)FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS:
    case (int)FABRIC_E_APPLICATION_TYPE_IN_USE:
    case (int)FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION:
    case (int)FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS:
    case (int)FABRIC_E_FABRIC_VERSION_IN_USE:
    case (int)FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS:
    case (int)FABRIC_E_NAME_ALREADY_EXISTS:
    case (int)FABRIC_E_NAME_NOT_EMPTY:
    case (int)FABRIC_E_PROPERTY_CHECK_FAILED:
    case (int)FABRIC_E_SERVICE_METADATA_MISMATCH:
    case (int)FABRIC_E_SERVICE_TYPE_MISMATCH:
    case (int)FABRIC_E_WRITE_CONFLICT:
    case (int)FABRIC_E_HEALTH_STALE_REPORT:
    case (int)FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED:
    case (int)FABRIC_E_INSTANCE_ID_MISMATCH:
    case (int)FABRIC_E_NODE_HAS_NOT_STOPPED_YET:
    case (int)FABRIC_E_REPAIR_TASK_ALREADY_EXISTS:
    case (int)FABRIC_E_SERVICE_TYPE_ALREADY_REGISTERED:
    case (int)FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS:
    case (int)FABRIC_E_HEALTH_MAX_REPORTS_REACHED:
    case (int)FABRIC_E_DUPLICATE_SERVICE_NOTIFICATION_FILTER_NAME:
    case (int)FABRIC_E_PRIMARY_ALREADY_EXISTS:
    case (int)FABRIC_E_SECONDARY_ALREADY_EXISTS:
    case (int)FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS:
    case (int)FABRIC_E_UPLOAD_SESSION_ID_CONFLICT:
    case (int)FABRIC_E_COMPOSE_DEPLOYMENT_ALREADY_EXISTS:
    case (int)FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH:
    case (int)FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS:
    case (int)FABRIC_E_NETWORK_IN_USE:
    {
        httpStatus = HttpStatusCode::Conflict;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_INVALID_PARTITION_KEY:
    case (int)FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR:
    case (int)FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR:
    case (int)FABRIC_E_INVALID_ADDRESS:
    case (int)FABRIC_E_APPLICATION_NOT_UPGRADING:
    case (int)FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR:
    case (int)FABRIC_E_FABRIC_NOT_UPGRADING:
    case (int)FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR:
    case (int)FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST:
    case (int)FABRIC_E_INVALID_CONFIGURATION:
    case (int)FABRIC_E_INVALID_CREDENTIAL_TYPE:
    case (int)FABRIC_E_INVALID_CREDENTIALS:
    case (int)FABRIC_E_INVALID_NAME_URI:
    case (int)FABRIC_E_INVALID_PROTECTION_LEVEL:
    case (int)FABRIC_E_INVALID_SUBJECT_NAME:
    case (int)FABRIC_E_INVALID_X509_FIND_TYPE:
    case (int)FABRIC_E_INVALID_X509_STORE:
    case (int)FABRIC_E_INVALID_X509_STORE_LOCATION:
    case (int)FABRIC_E_INVALID_X509_STORE_NAME:
    case (int)FABRIC_E_INVALID_X509_THUMBPRINT:
    case (int)FABRIC_E_PATH_TOO_LONG:
    case (int)FABRIC_E_KEY_TOO_LARGE:
    case (int)FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED:
    case (int)FABRIC_E_INVALID_ATOMIC_GROUP:
    case (int)FABRIC_E_VALUE_EMPTY:
    case (int)FABRIC_E_REPAIR_TASK_NOT_FOUND:
    case (int)FABRIC_E_USER_ROLE_CLIENT_CERTIFICATE_NOT_CONFIGURED:
    case (int)FABRIC_E_SERVICE_TYPE_NOT_REGISTERED:
    case (int)FABRIC_E_INVALID_OPERATION:
    case (int)FABRIC_E_TRANSACTION_NOT_ACTIVE:
    case (int)FABRIC_E_TRANSACTION_TOO_LARGE:
    case (int)FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND:
    case (int)FABRIC_E_INVALID_DIRECTORY:
    case (int)FABRIC_E_UPGRADE_DOMAIN_ALREADY_COMPLETED:
    case (int)FABRIC_E_INVALID_X509_NAME_LIST:
    case (int)FABRIC_E_INVALID_SERVICE_TYPE:
    case (int)FABRIC_E_REPLICATION_OPERATION_TOO_LARGE:
    case (int)FABRIC_E_INSUFFICIENT_CLUSTER_CAPACITY:
    case (int)FABRIC_E_CONSTRAINT_KEY_UNDEFINED:
    case (int)FABRIC_E_CONSTRAINT_NOT_SATISFIED:
    case (int)FABRIC_E_INVALID_PACKAGE_SHARING_POLICY:
    case (int)FABRIC_E_PREDEPLOYMENT_NOT_ALLOWED:
    case (int)FABRIC_E_MULTITHREADED_TRANSACTIONS_NOT_ALLOWED:
    case (int)FABRIC_E_CERTIFICATE_NOT_FOUND:
    case (int)FABRIC_E_INVALID_BACKUP_SETTING:
    case (int)FABRIC_E_INVALID_RESTORE_DATA:
    case (int)FABRIC_E_RESTORE_SAFE_CHECK_FAILED:
    case (int)FABRIC_E_DUPLICATE_BACKUPS:
    case (int)FABRIC_E_INVALID_BACKUP_CHAIN:
    case (int)FABRIC_E_INVALID_BACKUP:
    case (int)FABRIC_E_MISSING_FULL_BACKUP:
    case (int)FABRIC_E_INVALID_REPLICA_OPERATION:
    case (int)FABRIC_E_INVALID_REPLICA_STATE:
    case (int)FABRIC_E_BACKUP_IN_PROGRESS:
    case (int)FABRIC_E_INVALID_PARTITION_OPERATION:
    case (int)FABRIC_E_INVALID_TEST_COMMAND_STATE:
    case (int)FABRIC_E_INVALID_PARTITION_SELECTOR:
    case (int)FABRIC_E_INVALID_REPLICA_SELECTOR:
    case (int)FABRIC_E_CHAOS_ALREADY_RUNNING:
    case (int)FABRIC_E_STOP_IN_PROGRESS:
    case (int)FABRIC_E_ALREADY_STOPPED:
    case (int)FABRIC_E_NODE_IS_UP:
    case (int)FABRIC_E_NODE_IS_DOWN:
    case (int)FABRIC_E_NODE_TRANSITION_IN_PROGRESS:
    case (int)FABRIC_E_INVALID_INSTANCE_ID:
    case (int)FABRIC_E_INVALID_DURATION:
    case (int)FABRIC_E_UPLOAD_SESSION_RANGE_NOT_SATISFIABLE:
    case (int)FABRIC_E_INVALID_DNS_NAME:
    case (int)FABRIC_E_INVALID_FOR_STATEFUL_SERVICES:
    case (int)FABRIC_E_INVALID_FOR_STATELESS_SERVICES:
    case (int)FABRIC_E_ONLY_VALID_FOR_STATEFUL_PERSISTENT_SERVICES:
    case (int)FABRIC_E_INVALID_UPLOAD_SESSION_ID:
    case (int)FABRIC_E_BACKUP_NOT_ENABLED:
    case (int)FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST:
    case (int)FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED:
    case (int)FABRIC_E_INVALID_SERVICE_SCALING_POLICY:
    case (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_UPGRADING:
    case (int)FABRIC_E_VOLUME_NOT_FOUND:
    case (int)FABRIC_E_NETWORK_NOT_FOUND:
    case (int)FABRIC_E_ENDPOINT_NOT_REFERENCED:
    {
        httpStatus = HttpStatusCode::BadRequest;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_MESSAGE_TOO_LARGE:
    case (int)FABRIC_E_VALUE_TOO_LARGE:
    {
        httpStatus = HttpStatusCode::RequestEntityTooLarge;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_NODE_NOT_FOUND:
    case (int)FABRIC_E_APPLICATION_TYPE_NOT_FOUND:
    case (int)FABRIC_E_APPLICATION_NOT_FOUND:
    case (int)FABRIC_E_SERVICE_TYPE_NOT_FOUND:
    case (int)FABRIC_E_SERVICE_DOES_NOT_EXIST:
    case (int)FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND:
    case (int)FABRIC_E_PARTITION_NOT_FOUND:
    case (int)FABRIC_E_REPLICA_DOES_NOT_EXIST:
    case (int)FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST:
    case (int)FABRIC_E_ENDPOINT_NOT_FOUND:
    case (int)FABRIC_E_CODE_PACKAGE_NOT_FOUND:
    case (int)FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND:
    case (int)FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND:
    case (int)FABRIC_E_DATA_PACKAGE_NOT_FOUND:
    case (int)FABRIC_E_DIRECTORY_NOT_FOUND:
    case (int)FABRIC_E_FABRIC_VERSION_NOT_FOUND:
    case (int)FABRIC_E_FILE_NOT_FOUND:
    case (int)FABRIC_E_NAME_DOES_NOT_EXIST:
    case (int)FABRIC_E_PROPERTY_DOES_NOT_EXIST:
    case (int)FABRIC_E_ENUMERATION_COMPLETED:
    case (int)FABRIC_E_SERVICE_MANIFEST_NOT_FOUND:
    case (int)FABRIC_E_KEY_NOT_FOUND:
    case (int)FABRIC_E_HEALTH_ENTITY_NOT_FOUND:
    case (int)FABRIC_E_SERVICE_ENDPOINT_RESOURCE_NOT_FOUND:
    case (int)FABRIC_E_DNS_SERVICE_NOT_FOUND:
    case (int)FABRIC_E_COMPOSE_DEPLOYMENT_NOT_FOUND:
    {
        httpStatus = HttpStatusCode::NotFound;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_COMMUNICATION_ERROR:
    case (int)FABRIC_E_OPERATION_NOT_COMPLETE:
    case (int)FABRIC_E_TIMEOUT:
    {
        httpStatus = HttpStatusCode::GatewayTimeout;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_NO_WRITE_QUORUM:
    case (int)FABRIC_E_NOT_PRIMARY:
    case (int)FABRIC_E_NOT_READY:
    case (int)FABRIC_E_OBJECT_CLOSED:
    case (int)FABRIC_E_SERVICE_OFFLINE:
    case E_ABORT:
    case (int)FABRIC_E_CONNECTION_CLOSED_BY_REMOTE_END:
    case (int)FABRIC_E_RECONFIGURATION_PENDING:
    case (int)FABRIC_E_REPLICATION_QUEUE_FULL:
    case (int)FABRIC_E_SERVICE_TOO_BUSY:
    {
        httpStatus = HttpStatusCode::ServiceUnavailable;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_GATEWAY_NOT_REACHABLE:
    {
        httpStatus = HttpStatusCode::BadGateway;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case (int)FABRIC_E_SERVER_AUTHENTICATION_FAILED:
    {
        httpStatus = HttpStatusCode::Unauthorized;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    case ErrorCodeValue::NotFound:
    {
        httpStatus = HttpStatusCode::NotFound;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }

    default:
    {
        httpStatus = HttpStatusCode::InternalServerError;
        httpStatusLine = error.ErrorCodeValueToString();
        break;
    }
    }
}



