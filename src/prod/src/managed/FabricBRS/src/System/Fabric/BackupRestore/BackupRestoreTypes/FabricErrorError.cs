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
    using System.Fabric.BackupRestoreEndPoints;
    using System.Net.Http;

    /// <summary>
    ///Response Object for Fabric Backup Restore Error
    /// </summary>
    [DataContract]
    public class FabricErrorError
    {
        [DataMember]
        public FabricError Error;
        
        public FabricErrorError(FabricError error)
        {
            this.Error = error;
        }

        public HttpResponseMessage ToHttpResponseMessage()
        {
            HttpStatusCode httpStatusCode =  this.Error.ErrorCodeToHttpStatusCode();
            HttpResponseMessage httpResponseMessage = new HttpResponseMessage(httpStatusCode)
            {
                Content = new StringContent(JsonConvert.SerializeObject(this))
            };
            if (httpStatusCode == HttpStatusCode.NotFound)
            {
                httpResponseMessage.Headers.Add(BackupRestoreEndPointsConstants.Fabric404ErrorHeaderKey, BackupRestoreEndPointsConstants.Fabric404ErrorHeaderValue);
            }
            return httpResponseMessage;
        }
    }

  
}