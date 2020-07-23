// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Types
{
    using System.Fabric;
    using System.Net;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Fabric.Interop;

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

                case NativeTypes.FABRIC_ERROR_CODE.E_NOT_FOUND:
                    return HttpStatusCode.NotFound;

                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_COMMUNICATION_ERROR:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_OPERATION_NOT_COMPLETE:
                case NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT:
                    return HttpStatusCode.GatewayTimeout;

                default:
                    return HttpStatusCode.InternalServerError;
            }

        }
    }
}