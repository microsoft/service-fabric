// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreExceptions
{
    using Newtonsoft.Json;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;
    using Newtonsoft.Json.Converters;
    using System.Net;

    /// <summary>
    ///  Generic Fabric Backup Restore Exception
    /// </summary>
    [DataContract]
    public class FabricError
    {
        /// <summary>
        /// Error code associated with Fabric Error
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        internal NativeTypes.FABRIC_ERROR_CODE Code { set; get; }

        /// <summary>
        /// Message for the Fabric Error
        /// </summary>
        [DataMember]
        internal string Message { set; get; }

        public FabricError()
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="fabricException"></param>
        public FabricError(FabricException fabricException)
        {
            this.Code = (NativeTypes.FABRIC_ERROR_CODE)fabricException.ErrorCode;
            this.Message = fabricException.Message;
        }

        public HttpStatusCode ErrorCodeToHttpStatusCode()
        {
            switch (this.Code)
            {
                case NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG:
                
                    return HttpStatusCode.BadRequest;
                case NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED:
                    return HttpStatusCode.Forbidden;
                case NativeTypes.FABRIC_ERROR_CODE.E_NOTIMPL:
                    return HttpStatusCode.NotImplemented;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_ALREADY_IN_TARGET_VERSION:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_PROVISION_IN_PROGRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPGRADE_IN_PROGRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_GROUP_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_IN_USE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_ALREADY_IN_TARGET_VERSION:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_IN_USE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_UPGRADE_IN_PROGRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_NOT_EMPTY:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROPERTY_CHECK_FAILED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_METADATA_MISMATCH:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_MISMATCH:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_WRITE_CONFLICT:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_HEALTH_STALE_REPORT:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INSTANCE_ID_MISMATCH:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_HAS_NOT_STOPPED_YET:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPAIR_TASK_ALREADY_EXISTS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_IN_PROGRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RESTORE_IN_PROGRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_POLICY_ALREADY_EXISTS:
                    return HttpStatusCode.Conflict;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PARTITION_KEY:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_VALIDATION_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_RESERVED_DIRECTORY_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ADDRESS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_UPGRADING:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_UPGRADE_VALIDATION_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_NOT_UPGRADING:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_UPGRADE_VALIDATION_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ALLOWED_COMMON_NAME_LIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CONFIGURATION:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CREDENTIAL_TYPE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_CREDENTIALS:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_NAME_URI:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_PROTECTION_LEVEL:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_SUBJECT_NAME:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_FIND_TYPE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE_LOCATION:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_STORE_NAME:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_X509_THUMBPRINT:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PATH_TOO_LONG:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_TOO_LARGE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_AFFINITY_CHAIN_NOT_SUPPORTED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_ATOMIC_GROUP:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VALUE_EMPTY:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPAIR_TASK_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_IS_ENABLED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RESTORE_SOURCE_TARGET_PARTITION_MISMATCH:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_INVALID_FOR_STATELESS_SERVICES:
                    return HttpStatusCode.BadRequest;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_VALUE_TOO_LARGE:
                    return HttpStatusCode.RequestEntityTooLarge;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_TYPE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_TYPE_TEMPLATE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PARTITION_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_REPLICA_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ENDPOINT_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CODE_PACKAGE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIGURATION_PACKAGE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DATA_PACKAGE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_DIRECTORY_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_VERSION_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FILE_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NAME_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_PROPERTY_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_ENUMERATION_COMPLETED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_MANIFEST_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_HEALTH_ENTITY_NOT_FOUND:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_NOT_ENABLED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_BACKUP_POLICY_DOES_NOT_EXIST:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FAULT_ANALYSIS_SERVICE_NOT_ENABLED:
                    return HttpStatusCode.NotFound;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMMUNICATION_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OPERATION_NOT_COMPLETE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT:
                    return HttpStatusCode.GatewayTimeout;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NO_WRITE_QUORUM:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_PRIMARY:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OBJECT_CLOSED:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_OFFLINE:
                    return HttpStatusCode.ServiceUnavailable;

                default:
                    return HttpStatusCode.InternalServerError;
            }

        }
    }

    
    /// <summary>
    ///  
    /// </summary>
    public class FabricBackupRestoreLocalRetryException : Exception
    {
        
    }
}