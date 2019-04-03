// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    class CheckWorkStatusResponse
    {
        [JsonProperty(PropertyName = "WorkStatus")]
        [JsonConverter(typeof(StringEnumConverter))]
        public WorkStatus WorkStatus { get; set; }

        [JsonProperty(PropertyName = "StatusCode")]
        public long StatusCode { get; set; }
    }
}