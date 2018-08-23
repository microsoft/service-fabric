// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient
{
    using System.Fabric.Common;
    using System.Net.Http;
    using System.Text;

    internal class JsonRequestContent
    {
        private const string TraceType = "JsonRequestContent";
        private const string JsonMimeType = "application/json";
        
        internal static HttpContent GetContent<T>(T val)
        {
            var serializedObject = ContainerServiceClient.JsonSerializer.SerializeObject(val);

            var typeName = typeof(T).Name;
            AppTrace.TraceSource.WriteInfo(TraceType, $"{typeName} -- JSON -- {serializedObject}");

            return new StringContent(serializedObject, Encoding.UTF8, JsonMimeType);
        }
    }
}