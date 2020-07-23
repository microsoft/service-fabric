// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Types
{
    using System.Net;
    using System.Net.Http;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;

    /// <summary>
    ///Response Object for Fabric Backup Restore Error
    /// </summary>
    [DataContract]
    public class FabricErrorError
    {
        internal const string Fabric404ErrorHeaderKey = "X-ServiceFabric";
        internal const string Fabric404ErrorHeaderValue = "ResourceNotFound";

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
                httpResponseMessage.Headers.Add(Fabric404ErrorHeaderKey, Fabric404ErrorHeaderValue);
            }
            return httpResponseMessage;
        }
    }

  
}