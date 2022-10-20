// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using System;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    [Serializable]
    public class HttpClientRequest
    {
        [JsonProperty(PropertyName = "Operation")]
        [JsonConverter(typeof(StringEnumConverter))]
        public HttpClientAction Operation { get; set; }
    }
}